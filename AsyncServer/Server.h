#pragma once

#include "Session.h"
#include "Clientdb.h"

namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

class Server
{
private:
    tcp::acceptor m_acceptor;
	
    void do_accept(const ConnectData& cdata);

public:
    // нам нужен io_context и порт, который мы будем прослушивать
    Server(net::io_context& io_context, unsigned port, const ConnectData&& cdata);
};
