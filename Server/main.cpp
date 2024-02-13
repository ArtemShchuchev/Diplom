#include <iostream>
#include "Clientdb.h"
#include "ConfigFile.h"
#include "Server.h"
#include "SecondaryFunction.h"

int main()
{
	setRuLocale();

	try
	{
        ConfigFile config("../../config.ini");
        const unsigned serverPort(config.getConfig<unsigned>("httpServer", "port"));
        ConnectData connectDb;
        connectDb.dbname = config.getConfig<std::string>("BdConnect", "dbname");
        connectDb.host = config.getConfig<std::string>("BdConnect", "host");
        connectDb.password = config.getConfig<std::string>("BdConnect", "password");
        connectDb.username = config.getConfig<std::string>("BdConnect", "user");
        connectDb.port = config.getConfig<unsigned>("BdConnect", "port");
        ////////////////

		boost::asio::io_context io_context;
        Server s(io_context, serverPort, std::move(connectDb));
		std::wcout << L"В адресной строке браузера, введите http://localhost:" << serverPort << std::endl;
		io_context.run();
	}
    catch (const std::exception& err)
    {
        consoleCol(col::br_red);
        std::wcerr << L"\nИсключение типа: " << typeid(err).name() << '\n';
        std::wcerr << utf82wideUtf(err.what()) << '\n';
        consoleCol(col::cancel);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
