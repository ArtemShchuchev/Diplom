#include "HtmlClient.h"

std::wstring HtmlClient::do_request(std::string urlStr)
{
    try
    {
        // Парсинг строки ссылки
        boost::urls::url_view url;
        bool parseErr(true);
        do
        {
            auto parseResult = boost::urls::parse_uri(urlStr);
            url = parseResult.value();
            if (url.path().empty()) {
                urlStr += '/';
                parseErr = true;
            }
            else parseErr = false;
        } while (parseErr);
        ////////////////////////////////////////////
        //std::wcout << L"----------------\n";
        //std::wcout << L"URL parse... \n";
        //std::wcout << L"protocol: " << utf82wideUtf(url.scheme()) << "\n";
        //std::wcout << L"domain:   " << utf82wideUtf(url.host()) << "\n";
        //std::wcout << L"path:     " << utf82wideUtf(url.path()) << "\n";
        ////////////////////////////////////////////

        // Список конечных точек
        // host: en.wikipedia.org, scheme: https
        tcp::resolver resolver{ ioc };
        tcp::resolver::query query(url.host(), url.scheme());
        tcp::resolver::results_type sequenceEp = resolver.resolve(query);
        int port = sequenceEp->endpoint().port();
        ////////////////////////////////////////////
        //std::wcout << L"port:     " << port << '\n';
        ////////////////////////////////////////////

        // Настройка запроса HTTP GET
        http::request<http::string_body> request;
        request.method(http::verb::get);
        request.target(url.path()); // path: /wiki/Main_Page
        request.version(11);
        request.keep_alive(true);
        request.set(http::field::host, url.host()); // host: en.wikipedia.org
        request.set(http::field::user_agent, "Spider");
        ////////////////////////////////////////////
        //std::stringstream ss;
        //ss << request;
        //std::wcout << L"\nRequest: " << utf82wideUtf(ss.str());
        ////////////////////////////////////////////
        std::wstring str = (port == 443) ?
            httpsRequest(sequenceEp, request) :
            httpRequest(sequenceEp, request);

        return std::move(str);
    }
    catch (const boost::detail::with_throw_location<boost::system::system_error>)
    { /* Ошибка в синтаксисе URL -> игнор. */ }
    catch (const boost::wrapexcept<boost::system::system_error>)
    { /* Удаленный сервер, отверг соединение -> игнор. */ }
    catch (const std::exception& err)
    {
        consoleCol(col::br_red);
        std::wcerr << L"\nИсключение типа: " << typeid(err).name() << '\n';
        std::wcerr << L"Ссылка: " << urlStr << '\n';
        std::wcerr << L"Ошибка: " << utf82wideUtf(err.what()) << std::endl;
        consoleCol(col::cancel);
    }

    return L"";
}

std::wstring HtmlClient::httpsRequest(const tcp::resolver::results_type& sequenceEp,
    const http::request<http::string_body>& req)
{
    ssl::context ctx(ssl::context::sslv23);
    ctx.set_default_verify_paths();
    ctx.set_options(ssl::context::default_workarounds | ssl::context::verify_none);

    ssl::stream<tcp::socket> sslStream(ioc, ctx);
    sslStream.set_verify_mode(ssl::context::verify_none);
    sslStream.set_verify_callback([](bool, ssl::verify_context&) {return true; });
    ////////////////////////////////////////////
    //std::wcout << L">>> Подключение... ";
    ////////////////////////////////////////////
    boost::asio::connect(sslStream.lowest_layer(), sequenceEp);
    ////////////////////////////////////////////
    //std::wcout << L"успешно (" << sslStream.lowest_layer().remote_endpoint() << L") <<<\n";
    //std::wcout << L">>> Рукопожатие... ";
    ////////////////////////////////////////////
    sslStream.handshake(ssl::stream<tcp::socket>::client);
    ////////////////////////////////////////////
    //std::wcout << L"<<<\n";
    ////////////////////////////////////////////
    sslStream.lowest_layer().set_option(tcp::no_delay(true));
    ///////////////////////////////////////
    //std::wcout << L">>> Отправка... ";
    ////////////////////////////////////////////
    int bytes_sent = http::write(sslStream, req);
    ////////////////////////////////////////////
    //std::wcout << bytes_sent << L" байт отправлено <<<\n";
    ////////////////////////////////////////////
    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    ///////////////////////////////////////
    //std::wcout << L">>> Чтение... ";
    ////////////////////////////////////////////
    auto bytes_received = http::read(sslStream, buffer, res);
    ////////////////////////////////////////////
    //std::wcout << bytes_received << L" байт получено <<<\n";
    ////////////////////////////////////////////
    // Аккуратно закройте сокет
    beast::error_code ec;
    sslStream.shutdown(ec);
    if (ec == ssl::error::stream_truncated)
        ec = {};
    sslStream.lowest_layer().shutdown(tcp::socket::shutdown_both, ec);
    sslStream.lowest_layer().close();
    ////////////////////////////////////////////
    //std::wcout << ansi2wideUtf(ec.message()) << std::endl << std::endl;
    ////////////////////////////////////////////
    // not_connected иногда случается, так что не беспокойтесь об этом
    if (ec && ec != beast::errc::not_connected)
        throw beast::system_error{ ec };
        
    return checkResult(res);
}

std::wstring HtmlClient::httpRequest(const tcp::resolver::results_type& sequenceEp,
    const http::request<http::string_body>& req)
{
    beast::tcp_stream stream{ ioc };
    // Установите соединение по IP-адресу
    ////////////////////////////////////////////
    //std::wcout << L">>> Подключение... ";
    ////////////////////////////////////////////
    boost::asio::connect(stream.socket(), sequenceEp);
    // Отправьте HTTP-запрос на удаленный хост
    ////////////////////////////////////////////
    //std::wcout << L"успешно (" << stream.socket().lowest_layer().remote_endpoint() << L") <<<\n";
    //std::wcout << L">>> Отправка... ";
    ////////////////////////////////////////////
    int bytes_sent = http::write(stream, req);
    ////////////////////////////////////////////
    //std::wcout << bytes_sent << L" байт отправлено <<<\n";
    ////////////////////////////////////////////
    // Этот буфер используется для чтения и должен быть сохранен
    beast::flat_buffer buffer;
    // Объявите контейнер для хранения ответа
    http::response<http::dynamic_body> res;
    // Получите HTTP-ответ
    ///////////////////////////////////////
    //std::wcout << L">>> Чтение... ";
    ////////////////////////////////////////////
    int bytes_received = http::read(stream, buffer, res);
    ////////////////////////////////////////////
    //std::wcout << bytes_received << L" байт получено <<<\n";
    ////////////////////////////////////////////
    // Аккуратно закройте сокет
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    stream.socket().close();
    ////////////////////////////////////////////
    //std::wcout << ansi2wideUtf(ec.message()) << std::endl << std::endl;
    ////////////////////////////////////////////
    // not_connected иногда случается, так что не беспокойтесь об этом
    if (ec && ec != beast::errc::not_connected)
        throw beast::system_error{ ec };

    return checkResult(res);
}

std::wstring HtmlClient::checkResult(http::response<http::dynamic_body> res)
{
    std::wstring ws;
    unsigned int responseCode(res.base().result_int());
    switch (responseCode)
    {
        case 301:
        {
            std::string url(res.base()["Location"]);
            ////////////////////////////////////////////
            //std::wcout << responseCode << L": Перенаправлено: " << ansi2wideUtf(url) << "\n\n";
            ////////////////////////////////////////////
            ws = do_request(url);
            break;
        }
        case 200:
        {
            std::stringstream ss;
            ss << res;
            std::string s(ss.str());
            ws = utf82wideUtf(s);
            break;
        }
        default:
            ////////////////////////////////////////////
            //std::wcout << L"Unexpected HTTP status " << responseCode << "\n\n";
            ////////////////////////////////////////////
            break;
    }

    return ws;
}

std::wstring HtmlClient::getRequest(const std::string& urlStr)
{
    return do_request(urlStr);
}
