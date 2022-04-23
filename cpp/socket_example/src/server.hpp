#pragma once

#include <iostream>
#include <thread>
#include <future>
#include <boost/asio/ip/tcp.hpp>

namespace socket_example
{

class Server
{
public:
Server(std::string ip_, uint16_t port_);
virtual ~Server();

void start();
std::future_status status() const;

private:
    struct Connection
    {
        boost::asio::io_context context_io;
        std::thread _thread;
        std::shared_ptr<boost::asio::ip::tcp::socket> socket;
        std::vector<char> rx_buffer; 
        std::vector<char> tx_buffer; 
    };

    boost::asio::ip::tcp::endpoint server_endpoint;
    boost::asio::io_context io;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;

    // std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>> new_connections;
    std::vector<std::shared_ptr<Connection>> connections;
    std::mutex rx_mutex;

    std::future<void> status_future;

    void start_up();
    // void acceptor_callback(const boost::system::error_code& ec, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
    // void rx_callback(const boost::system::error_code& ec, size_t bytes, std::shared_ptr<boost::asio::ip::tcp::socket> client_socket);
    
    // void acceptor_callback(const boost::system::error_code& ec, std::shared_ptr<Connection> client_connection);
    void rx_callback(const boost::system::error_code& ec, size_t bytes, std::shared_ptr<Connection> client_connection);
};

}
