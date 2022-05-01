#pragma once

#include <iostream>
#include <thread>
#include <future>
#include <boost/asio/ip/tcp.hpp>
#include "types.hpp"

namespace tcp
{

class Server
{
public:
Server(std::string ip_, uint16_t port_, std::function<void(uint16_t clientPort, std::unique_ptr<Payload> rxBuffer_)> handler_ = nullptr);
virtual ~Server();

void start();
std::future_status status() const;

void send(uint16_t clientPort, std::unique_ptr<Payload> txBuffer_);

private:
    class Connection
    {
        public:
        Connection();
        ~Connection();

        void start();
        Socket& getSocket(); 
        Payload& getRxBuffer();
        Payload& getTxBuffer();

        private:
        Context context_io;
        std::thread _thread;
        std::shared_ptr<Socket> socket;
        Payload rx_buffer; 
        Payload tx_buffer; 
    };

    Endpoint server_endpoint;
    Context io;
    std::unique_ptr<Acceptor> acceptor;

    std::map<uint16_t, std::shared_ptr<Connection>> connections;
    std::mutex rx_mutex;

    std::future<void> status_future;

    std::function<void(uint16_t clientPort, std::unique_ptr<Payload> rxBuffer_)> handler;

    void start_up();
    void rx_callback(const boost::system::error_code& ec, size_t bytes, std::shared_ptr<Connection> client_connection);
};

}
