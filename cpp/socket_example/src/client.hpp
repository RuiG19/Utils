#pragma once

#include <iostream>
#include <thread>
#include <future>
#include <boost/asio/ip/tcp.hpp>

namespace socket_example
{

class Client
{
public:
Client(std::string ip_, uint16_t port_, std::string server_ip_, uint16_t server_port_);
virtual ~Client();

void start();
std::future_status status() const;
uint16_t getId() const;

private:
    static uint16_t _id_generator;
    uint16_t id;
    const std::string client_id;

    boost::asio::io_context io;
    boost::asio::ip::tcp::endpoint client_endpoint;
    boost::asio::ip::tcp::endpoint server_endpoint;

    std::future<void> status_future;

    std::vector<char> rx_buffer; 
    std::vector<char> tx_buffer; 

    std::shared_ptr<boost::asio::ip::tcp::socket> server_socket;

    void start_up();
    void rx_callback(const boost::system::error_code& ec, size_t bytes);

};

}

