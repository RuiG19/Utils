#pragma once

#include <vector>
#include <boost/asio/ip/tcp.hpp>


namespace tcp
{

// STL types
using Payload = std::vector<uint8_t>;

// Boost types
using Context = boost::asio::io_context;
using Endpoint = boost::asio::ip::tcp::endpoint;
using Socket = boost::asio::ip::tcp::socket;
using Acceptor = boost::asio::ip::tcp::acceptor;

}
