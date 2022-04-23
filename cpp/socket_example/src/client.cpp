#include "client.hpp"
#include "logger.hpp"

namespace socket_example
{

uint16_t Client::_id_generator = 0;

Client::Client(std::string ip_, uint16_t port_, std::string server_ip_, uint16_t server_port_) 
              : id(++_id_generator), client_id("Client_" + std::to_string(id))
{
    client_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address_v4(ip_), port_);
    server_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address_v4(server_ip_), server_port_);
    tx_buffer = {'P','I','N','G'}; 
    rx_buffer = std::vector<char>(4090); 
}

Client::~Client()
{
    std::string function_id = getFunctionId(__func__, client_id);

    rx_buffer.resize(0);
    tx_buffer.resize(0);

    try
    {
        server_socket->cancel();
        server_socket->close();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << function_id << e.what();
    }
    
}

void Client::start()
{
    status_future = std::async(std::launch::async, [=](){this->start_up(); });
}

uint16_t Client::getId() const
{
    return id;
}

std::future_status Client::status() const
{
    return status_future.wait_for(std::chrono::milliseconds(0));
}

void Client::start_up()
{
    std::string function_id = getFunctionId(__func__, client_id);

    LOG_DEBUG << function_id << " Starting CLIENT thread";

    server_socket = std::make_shared<boost::asio::ip::tcp::socket>(io);

    try
    {
        LOG_DEBUG << function_id <<  " OPEN ip_v4 socket";
        server_socket->open(client_endpoint.protocol().v4());

        LOG_DEBUG << function_id <<  " SET_OPTION reuse_address(true)";
        server_socket->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

        LOG_DEBUG << function_id <<  " BIND [" << client_endpoint.address().to_string() << ":" 
                              << client_endpoint.port() << "]";
        server_socket->bind(client_endpoint);

        LOG_DEBUG << function_id <<  " CONNECT TO [" << server_endpoint.address().to_string() << ":" 
                              << server_endpoint.port() << "]";
        server_socket->connect(server_endpoint);

        LOG_DEBUG << function_id <<  " Setting Async Rx Callback";
        server_socket->async_receive(boost::asio::buffer(rx_buffer), [=](const boost::system::error_code& ec, size_t bytes){this->rx_callback(ec, bytes); });

        LOG_DEBUG << function_id <<  " Sending PING to Server(" << server_socket->remote_endpoint().port() << ")";
        server_socket->send(boost::asio::buffer(tx_buffer));

        LOG_DEBUG << function_id <<  " Starting io_context run";
        io.run();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << function_id << " " << e.what();
        exit (EXIT_FAILURE);
    }
    

}

void Client::rx_callback(const boost::system::error_code& ec, size_t bytes)
{
    std::string function_id = getFunctionId(__func__, client_id);
    LOG_DEBUG << function_id <<  " Got something!";
    
    if(ec)
    {
        LOG_WARNING << function_id <<  " Erro code: " << ec.message();
        return;
    }

    if(bytes)
    {
        LOG_DEBUG << function_id <<  " Received " << bytes << " bytes";
        std::string payload(rx_buffer.begin(), rx_buffer.end());
        LOG_DEBUG << function_id <<  " Rx Payload: " << payload;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    LOG_DEBUG << function_id <<  " Setting Async Rx Callback";
    server_socket->async_receive(boost::asio::buffer(rx_buffer), [=](const boost::system::error_code& ec, size_t bytes){this->rx_callback(ec, bytes); });

    LOG_DEBUG << function_id <<  " Sending PING to Server(" << server_socket->remote_endpoint().port() << ")";
    server_socket->send(boost::asio::buffer(tx_buffer));
    io.run();

}


}

