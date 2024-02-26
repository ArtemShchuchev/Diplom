#pragma once

#include <iostream>
#include <boost/beast.hpp>
#include <boost/url.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "SecondaryFunction.h"

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
namespace ssl = net::ssl;
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

using Message = std::pair<unsigned int, std::wstring>;

namespace HtmlClient
{
	Message getRequest(const std::string& urlStr);
};
