#pragma once

#include <iostream>
#include <thread>
#include <future>
#include <boost/asio/ip/tcp.hpp>
#include "types.hpp"


namespace tcp
{

class Client
{
public:
Client(std::string ip_, uint16_t port_, std::string server_ip_, uint16_t server_port_, std::function<void(std::unique_ptr<Payload> rxBuffer_)> handler_ = nullptr);
virtual ~Client();

void start();
std::future_status status() const;
uint16_t getId() const;

void send(std::unique_ptr<Payload> txBuffer_);


private:
    static uint16_t _id_generator;
    uint16_t id;
    const std::string client_id;

    Context io;
    Endpoint client_endpoint;
    Endpoint server_endpoint;

    std::future<void> status_future;

    Payload rx_buffer; 
    Payload tx_buffer; 

    std::shared_ptr<Socket> server_socket;

    std::function<void(std::unique_ptr<Payload> rxBuffer_)> handler;

    void start_up();
    void rx_callback(const boost::system::error_code& ec, size_t bytes);

};

}

