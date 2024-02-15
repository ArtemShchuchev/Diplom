#include "Session.h"

Session::Session(tcp::socket socket, const ConnectData& cdata)
	: m_socket(std::move(socket)), m_cdata(cdata) { }

void Session::run()
{
	waitForRequest();
	checkDeadline();
}

void Session::waitForRequest()
{
	// т.к. нужно запомнить `this` для обратного вызова, нам нужно вызвать shared_from_this()
	auto self(shared_from_this());
	// как только прибудут данные, будет вызывана лямбда
	http::async_read(m_socket, m_buffer, m_request,
		[this, self](boost::system::error_code ec, std::size_t /*length*/)
		{
			// если ошибки не было, значит все прошло хорошо
			if (!ec) {
				self->processingRequest();
			} else {
				consoleCol(col::br_red);
				std::wcerr << L"Ошибка сессии: " << ansi2wideUtf(ec.message()) << std::endl;
				consoleCol(col::cancel);
			}
		});
}

void Session::processingRequest()
{
	m_response.version(m_request.version());
	m_response.keep_alive(false);

	switch (m_request.method())
	{
	case http::verb::get:
		m_response.result(http::status::ok);
		m_response.set(http::field::server, "Beast");
		createResponseGet();
		break;
	
	case http::verb::post:
		m_response.result(http::status::ok);
		m_response.set(http::field::server, "Beast");
		createResponsePost();
		break;

	default:
		m_response.result(http::status::bad_request);
		m_response.set(http::field::content_type, "text/plain");
		beast::ostream(m_response.body())
			<< "Invalid request-method '"
			<< std::string(m_request.method_string())
			<< "'";
		break;
	}

	writeResponse();
}

void Session::createResponseGet()
{
	if (m_request.target() == "/")
	{
		m_response.set(http::field::content_type, "text/html");
		beast::ostream(m_response.body())
			<< "<html>\n"
			<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
			<< "<body>\n"
			<< "<h1>Поисковый движок</h1>\n"
			<< "<p>Добро пожаловать!<p>\n"
			<< "<form action=\"/\" method=\"post\">\n"
			<< "    <label for=\"search\">Введите слова для поиска</label><br>\n"
			<< "    <input type=\"text\" id=\"search\" name=\"search\"><br>\n"
			<< "    <input type=\"submit\" value=\"Искать...\">\n"
			<< "</form>\n"
			<< "</body>\n"
			<< "</html>\n";
	}
	else
	{
		m_response.result(http::status::not_found);
		m_response.set(http::field::content_type, "text/plain");
		beast::ostream(m_response.body()) << "File not found\r\n";
	}
}

void Session::createResponsePost()
{
	if (m_request.target() == "/")
	{
		std::string s = url_decode(buffers_to_string(m_request.body().data()));

		size_t pos = s.find('=');
		if (pos == std::string::npos)
		{
			m_response.result(http::status::not_found);
			m_response.set(http::field::content_type, "text/plain");
			beast::ostream(m_response.body()) << "File not found\r\n";
			return;
		}

		std::string key = s.substr(0, pos);
		if (key != "search")
		{
			m_response.result(http::status::not_found);
			m_response.set(http::field::content_type, "text/plain");
			beast::ostream(m_response.body()) << "File not found\r\n";
			return;
		}

		// выделил искомые слова
		std::vector<std::string> word;
		unsigned count(0);
		while (true)
		{
			if (++count > 4) break;	// не больше 4х слов
			s = s.substr(pos + 1);
			pos = s.find('+');
			if (pos != std::string::npos) {
				word.push_back(s.substr(0, pos));
			}
			else {
				if (!s.empty()) word.push_back(s);
				break;
			}
		}

		// Достаю из БД ссылки и повторяемость для искомого слова
		if (word.empty() == false) {
			std::unordered_map<std::string, unsigned> LinkAmount;
			try
			{
				Clientdb db(m_cdata);
				LinkAmount = db.getLinkAmount(word);
			}
			catch (const pqxx::broken_connection& err)
			{
				std::wstring err_str(L"Ошибка подключения к PostgreSQL: " + ansi2wideUtf(err.what()));
				throw std::runtime_error(wideUtf2utf8(err_str));
			}
			catch (const std::exception& err)
			{
				std::string err_str("Ошибка PostgreSQL: ");
				throw std::runtime_error(err_str + err.what());
			}

			if (LinkAmount.empty())
			{
				m_response.set(http::field::content_type, "text/html");
				beast::ostream(m_response.body())
					<< "<html>\n"
					<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
					<< "<body>\n"
					<< "<h1>Поисковый движок</h1>\n"
					<< "<p>К сожалению, ничего не нашлось. (<p>\n"
					<< "<a href = \"..\\\">"
					<< "Вернуться назад" << "</a>\n"
					<< "</body>\n"
					<< "</html>\n";
			}
			else
			{
				// сортирую по убыванию повторяемости:
				std::vector<std::pair<std::string, unsigned>> linksSortedByAmount(
					LinkAmount.begin(),
					LinkAmount.end()
				);
				std::sort(
					linksSortedByAmount.begin(),
					linksSortedByAmount.end(),
					[](const auto& p1, const auto& p2) {
						return p1.second > p2.second;
					}
				);

				m_response.set(http::field::content_type, "text/html");
				beast::ostream(m_response.body())
					<< "<html>\n"
					<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
					<< "<body>\n"
					<< "<h1>Поисковый движок</h1>\n"
					<< "<p>Ссылки содержашие искомую фразу:<p>\n"
					<< "<ul>\n";
				unsigned count(0);
				for (const auto& [url, amount] : linksSortedByAmount) {
					if (++count > 10) break;	// не более 10 ссылок
					beast::ostream(m_response.body())
						<< "<li><a href=\""
						<< url << "\">"
						<< url << "</a></li>";
				}
				beast::ostream(m_response.body())
					<< "</ul>\n"
					<< "<a href = \"..\\\">"
					<< "Вернуться назад" << "</a>\n"
					<< "</body>\n"
					<< "</html>\n";
			}
		}
		else
		{
			m_response.set(http::field::content_type, "text/html");
			beast::ostream(m_response.body())
				<< "<html>\n"
				<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
				<< "<body>\n"
				<< "<h1>Поисковый движок</h1>\n"
				<< "<p>Добро пожаловать!<p>\n"
				<< "<form action=\"/\" method=\"post\">\n"
				<< "    <label for=\"search\">Введите слова для поиска</label><br>\n"
				<< "    <input type=\"text\" id=\"search\" name=\"search\"><br>\n"
				<< "    <input type=\"submit\" value=\"Искать...\">\n"
				<< "\n<h3>Нужно ввести хоть одно слово!</h3>\n"
				<< "</form>\n"
				<< "</body>\n"
				<< "</html>\n";
		}
	}
	else
	{
		m_response.result(http::status::not_found);
		m_response.set(http::field::content_type, "text/plain");
		beast::ostream(m_response.body()) << "File not found\r\n";
	}
}

void Session::writeResponse()
{
	auto self(shared_from_this());

	m_response.content_length(m_response.body().size());

	http::async_write(m_socket, m_response,
		[self](beast::error_code ec, std::size_t)
		{
			self->m_socket.shutdown(tcp::socket::shutdown_send, ec);
			self->m_deadline.cancel();
		});
}

void Session::checkDeadline()
{
	auto self(shared_from_this());

	m_deadline.async_wait(
		[self](beast::error_code ec) {
			if (!ec) {
				self->m_socket.close(ec);
			}
		});
}
