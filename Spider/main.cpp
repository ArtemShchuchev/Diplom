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
    std::mutex parse;
    std::mutex db;
};
auto lock = std::make_shared<Lock>();
auto const HARD_CONCUR(std::thread::hardware_concurrency());
Thread_pool threadPool(HARD_CONCUR);
static void spiderTask(const Link url, std::shared_ptr<Lock> lock,
    Thread_pool& threadPool, ConnectData& conData);

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

        spiderTask(url, lock, threadPool, connectDb);
        // таймаут каждого потока, после чего он считается "зависшим"
        threadPool.setTimeout(std::chrono::seconds(60));
        threadPool.wait();
    }
    catch (const std::exception& err)
    {
        std::lock_guard<std::mutex> lg(lock->console);
        consoleCol(col::br_red);
        std::wcerr << L"\nИсключение типа: " << typeid(err).name() << '\n';
        std::wcerr << utf82wideUtf(err.what()) << '\n';
        consoleCol(col::cancel);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


static void spiderTask(const Link url, std::shared_ptr<Lock> lock,
    Thread_pool& threadPool, ConnectData& conData)
{
    std::string url_str;
    std::unique_lock<std::mutex> ul_console(lock->console, std::defer_lock);

    try
    {
        url_str = url_decode(url.link_str);

        // проверяю наличие ссылки в БД
        std::unique_lock<std::mutex> ul_db(lock->db);
        auto db_ptr = std::make_unique<Clientdb>(conData);
        int idLink = db_ptr->getIdLink(url_str);

        if (idLink == 0) {
            idLink = db_ptr->addLink(url_str);
            ul_db.unlock();
        }
        else {
            ul_db.unlock();
            ul_console.lock();
            consoleCol(col::blue);
            std::wcout << L"Уже есть, пропускаю: " << utf82wideUtf(url_str) << L'\n';
            consoleCol(col::cancel);
            ul_console.unlock();
            //return; // url уже есть в базе
        }

        // Загрузка очередной странички
        std::wstring page = HtmlClient::getRequest(url.link_str); // url -> page

        // Поиск слов/ссылок на страничке
        if (page.empty() == false) {
            // page, recurse -> word, amount, listLink
            auto [words, links](WordSearch::getWordLink(std::move(page), url.recLevel, lock->parse));
                
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
                    threadPool.add([link, lock, &threadPool, &conData]
                        { spiderTask(link, lock, threadPool, conData); });
                }
            }

            // вывод в консоль
            std::wstring message_str(L"(" + std::to_wstring(url.recLevel) + L")(url:");
            message_str += std::to_wstring(links.size()) + L")(word:";
            message_str += std::to_wstring(words.size()) + L") ";
            message_str += utf82wideUtf(url_str) + L'\n';
            ul_console.lock();
            std::wcout << message_str;
            ul_console.unlock();

            // Сохранение найденных слов/ссылок в БД
            if (words.empty() == false) {
                ul_db.lock();
                idWordAm_vec idWordAm(db_ptr->addWords(std::move(words)));
                db_ptr->addLinkWords(idLink, idWordAm);
                return; // Работа проделана - выходим
            }
        }

        // что то пошло не так -> пустая ссылка не нужна
        ul_db.lock();
        db_ptr->deleteLink(idLink);
    }
    catch (pqxx::broken_connection& err)
    {
        std::wstring err_str(L"Ошибка pqxx::broken_connection!\n"
            + ansi2wideUtf(err.what()));

        std::rethrow_exception(
            std::make_exception_ptr(
                std::runtime_error(wideUtf2utf8(err_str))));
    }
    catch (const pqxx::data_exception& err)
    {
        ul_console.lock();
        consoleCol(col::br_red);
        std::wcerr << L"Исключение типа: " << typeid(err).name() << '\n';
        std::wcerr << L"Ссылка: " << utf82wideUtf(url_str) << '\n';
        std::wcerr << L"Ошибка: " << utf82wideUtf(err.what()) << std::endl;
        consoleCol(col::cancel);
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
