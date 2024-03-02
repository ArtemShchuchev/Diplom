#include <iostream>
#include "HtmlClient.h"
#include "Clientdb.h"
#include "ConfigFile.h"
#include "wordSearch.h"
#include "Thread_pool.h"
#include "SecondaryFunction.h"
#include "../Types.h"
#include "UrlEncodeDecode.h"


struct Lock {
    std::mutex console;
    std::mutex db;
};
auto lock = std::make_shared<Lock>();
auto const HARD_CONCUR(std::thread::hardware_concurrency());
Thread_pool threadPool(HARD_CONCUR);
class CacheLink {
private:
    std::mutex m;
    // несортированное множиство в качестве кэша ссылок
    std::unordered_set<std::string> links;

public:
    void set(std::unordered_set<std::string>& link) {
        links.swap(link);
    }
    bool addLink(const std::string& link) {
        std::lock_guard<std::mutex> lg(m);
        auto result = links.insert(link);
        // true -> url внесен в кэш
        return result.second;
    }
    void deleteLink(const std::string& link) {
        std::lock_guard<std::mutex> lg(m);
        links.erase(link);
    }
}cachelink;

static void spiderTask(const Link url, std::shared_ptr<Lock> lock,
    Thread_pool& threadPool, std::shared_ptr<Clientdb> db_ptr);

int main(int argc, char** argv)
{
    setRuLocale();
    consoleClear();

    try
    {
	    ConfigFile config("../../config.ini");
        std::string firstLink(config.getConfig<std::string>("Spider", "startWeb"));
        if (firstLink.empty()) {
            throw std::logic_error("Файл конфигурации не содержит ссылки!");
        }
        unsigned int recurse(config.getConfig<unsigned int>("Spider", "recurse"));
        firstLink = url_encode(firstLink);
        Link url{ std::move(firstLink), recurse };

        ConnectData connectDb;
        connectDb.dbname = config.getConfig<std::string>("BdConnect", "dbname");
        connectDb.host = config.getConfig<std::string>("BdConnect", "host");
        connectDb.password = config.getConfig<std::string>("BdConnect", "password");
        connectDb.username = config.getConfig<std::string>("BdConnect", "user");
        connectDb.port = config.getConfig<unsigned>("BdConnect", "port");

        // наполняю кэш ссылками из БД
        auto db_ptr = std::make_shared<Clientdb>(connectDb);
        cachelink.set(db_ptr->getLinks());

        spiderTask(url, lock, threadPool, db_ptr);
        // таймаут каждого потока, после чего он считается "зависшим"
        threadPool.setTimeout(std::chrono::seconds(60));
        threadPool.wait();
    }
    catch (const pqxx::broken_connection& err)
    {
        std::lock_guard<std::mutex> lg(lock->console);
        consoleCol(col::br_red);
        std::wcerr << L"\nИсключение типа: " << typeid(err).name() << '\n';
        std::wcerr << ansi2wideUtf(err.what()) << std::endl;
        consoleCol(col::cancel);
        return EXIT_FAILURE;
    }
    catch (const pqxx::sql_error& err)
    {
        std::lock_guard<std::mutex> lg(lock->console);
        consoleCol(col::br_red);
        std::wcerr << L"\nИсключение типа: " << typeid(err).name() << '\n';
        std::wcerr << utf82wideUtf(err.what()) << std::endl;
        consoleCol(col::cancel);
        return EXIT_FAILURE;
    }
    catch (const std::exception& err)
    {
        std::lock_guard<std::mutex> lg(lock->console);
        consoleCol(col::br_red);
        std::wcerr << L"\nИсключение типа: " << typeid(err).name() << '\n';
        std::wcerr << utf82wideUtf(err.what()) << std::endl;
        consoleCol(col::cancel);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


static void spiderTask(const Link url, std::shared_ptr<Lock> lock,
    Thread_pool& threadPool, std::shared_ptr<Clientdb> db_ptr)
{
    std::string url_str;
    std::unique_lock<std::mutex> ul_console(lock->console, std::defer_lock);

    try
    {
        url_str = url_decode(url.link_str);

        // проверяю наличие ссылки в кэше БД
        if (!cachelink.addLink(url_str)) { // url уже есть в базе
            ul_console.lock();
            consoleCol(col::blue);
            std::wcout << L"Уже есть, пропускаю: " << utf82wideUtf(url_str) << std::endl;
            consoleCol(col::cancel);
            ul_console.unlock();
            return;
        }

        // Загрузка очередной странички
        std::wstring page;
        auto[errCode, message] = HtmlClient::getRequest(url.link_str); // url -> page
        switch (errCode)
        {
        case 200:// all ok
            page = std::move(message);
            break;

        case 301:
        {
            Link link;
            link.recLevel = url.recLevel;
            link.link_str = wideUtf2utf8(message);
            threadPool.add([link, lock, &threadPool, db_ptr]
                { spiderTask(link, lock, threadPool, db_ptr); });
            break;
        }

        case 1:
        default:
            ul_console.lock();
            consoleCol(col::br_yellow);
            std::wcout << utf82wideUtf(url_str) << L'\n';
            std::wcout << message << std::endl;
            consoleCol(col::cancel);
            ul_console.unlock();
            break;
        }

        // Поиск слов/ссылок на страничке
        if (page.empty() == false) {
            // page, recurse -> word, amount, listLink
            auto [words, links](WordSearch::getWordLink(std::move(page), url.recLevel));
                
            // добавление задач в очередь
            if (links.empty() == false) {
                for (auto& link : links) {
                    if (link.link_str[0] == '/') {
                        // Парсинг строки ссылки
                        boost::url urlParse = boost::urls::parse_uri(url.link_str).value();

                        std::string rootLink = urlParse.scheme();
                        if (link.link_str[1] == '/') rootLink += ":";
                        else {
                            rootLink += "://";
                            rootLink += urlParse.host();
                        }

                        link.link_str = rootLink + link.link_str;
                    }
                    else if (link.link_str.find("http") != 0) continue;
                    threadPool.add([link, lock, &threadPool, db_ptr]
                        { spiderTask(link, lock, threadPool, db_ptr); });
                }
            }

            // вывод в консоль
            std::wstring message_str(L"(" + std::to_wstring(url.recLevel) + L")(url:");
            message_str += std::to_wstring(links.size()) + L")(word:";
            message_str += std::to_wstring(words.size()) + L") ";
            message_str += utf82wideUtf(url_str);
            ul_console.lock();
            std::wcout << message_str << std::endl;;
            ul_console.unlock();

            // Сохранение найденных слов/ссылок в БД
            if (words.empty() == false) {
                std::unique_lock<std::mutex> ul_db(lock->db);
                int idLink = db_ptr->addLink(url_str);
                idWordAm_vec idWordAm(db_ptr->addWords(std::move(words)));
                db_ptr->addLinkWords(idLink, idWordAm);
                ul_db.unlock();
                return; // Работа проделана - выходим
            }
        }

        // что то пошло не так -> пустая ссылка не нужна
        cachelink.deleteLink(url_str);
    }
    catch (const pqxx::protocol_violation&)
    {
        std::rethrow_exception(std::current_exception());

    }
    catch (const pqxx::broken_connection& err)
    {
        std::wstring err_str(L"Ошибка pqxx::broken_connection!\n"
            + ansi2wideUtf(err.what()));

        std::rethrow_exception(
            std::make_exception_ptr(
                std::runtime_error(wideUtf2utf8(err_str))));
    }
    catch (const std::exception& err)
    {
        ul_console.lock();
        consoleCol(col::br_red);
        std::wcerr << L"Исключение типа: " << typeid(err).name() << L'\n';
        std::wcerr << L"Ссылка: " << utf82wideUtf(url_str) << L'\n';
        std::wcerr << L"Ошибка: " << utf82wideUtf(err.what()) << std::endl;
        consoleCol(col::cancel);
    }
}
