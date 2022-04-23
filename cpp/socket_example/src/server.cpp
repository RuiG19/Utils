#include "server.hpp"
#include "logger.hpp"

namespace socket_example
{

Server::Server(std::string ip_, uint16_t port_)
{
    // io = boost::asio::io_context();

    server_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address_v4(ip_), port_);
}

Server::~Server()
{
    std::string function_id = getFunctionId(__func__, "Server");

    connections.resize(0); // FIXME: mem leak on the rx and tx buffers of each connection.

    try
    {
        acceptor->cancel();
        acceptor->close();
        // new_connection->cancel();
        // new_connection->close();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << function_id << e.what();
    }
}

void Server::start()
{
    status_future = std::async(std::launch::async, [=](){this->start_up();});
}

std::future_status Server::status() const
{
    return status_future.wait_for(std::chrono::milliseconds(0));
}

void Server::start_up()
{
    std::string function_id = getFunctionId(__func__, "Server");

    LOG_DEBUG << function_id <<  " Starting SERVER thread";
    // boost::asio::io_context io;
    acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(io);

    try
    {
        LOG_DEBUG << function_id <<  " OPEN ip_v4 socket";
        acceptor->open(server_endpoint.protocol().v4());

        LOG_DEBUG << function_id <<  " SET_OPTION reuse_address(true)";
        acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        
        LOG_DEBUG << function_id <<  " BIND [" << server_endpoint.address().to_string() << ":" 
                              << server_endpoint.port() << "]";
        acceptor->bind(server_endpoint);

        LOG_DEBUG << function_id <<  " LISTEN start";
        acceptor->listen(boost::asio::socket_base::max_connections);    

        while(true)
        {
            if (acceptor->is_open())
            {
                LOG_DEBUG << function_id <<  " ACCEPTOR is open ... start waiting for a new connections";

                std::shared_ptr<Connection> connection = std::make_shared<Connection>();
                connections.push_back(connection);
                
                connection->tx_buffer = {'P','O','N','G'}; 
                connection->rx_buffer = std::vector<char>(4090); 
                connection->socket = std::make_shared<boost::asio::ip::tcp::socket>(connection->context_io);
                
                acceptor->accept(*(connection->socket));

                LOG_DEBUG << function_id <<  "New connection accepted with Client(" << connection->socket->remote_endpoint().port() << ")";
                LOG_DEBUG << function_id <<  " Setting Async Rx Callback for Client(" << connection->socket->remote_endpoint().port() << ")";
                connection->socket->async_receive(boost::asio::buffer(connection->rx_buffer), [=](const boost::system::error_code& ec, size_t bytes)
                {
                    rx_callback(ec, bytes, connection);
                });
                
                connection->_thread = std::thread([=](){ connection->context_io.run(); });

                LOG_DEBUG << function_id <<  " Starting io_context run";
                io.run();
                
            }
            else
            {
                LOG_ERROR << function_id <<  " ACCEPTOR is closed";
                exit (EXIT_FAILURE);
            }
        }
    
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << function_id << " " << e.what();
        exit (EXIT_FAILURE);
    }    

}

void Server::rx_callback(const boost::system::error_code& ec, size_t bytes, std::shared_ptr<Connection> client_connection)
{
    std::lock_guard<std::mutex> lock(rx_mutex);

    std::string function_id = getFunctionId(__func__, "Server");
    LOG_DEBUG << function_id <<  " Got something from Client(" << client_connection->socket->remote_endpoint().port() << ")";
    
    if(ec)
    {
        LOG_WARNING << function_id <<  " Erro code: " << ec.message();
        // return;
    }

    if(bytes)
    {
        LOG_DEBUG << function_id <<  " Received " << bytes << " bytes";
        std::string payload(client_connection->rx_buffer.begin(), client_connection->rx_buffer.end());
        LOG_DEBUG << function_id <<  " Rx Payload: " << payload;

        LOG_DEBUG << function_id <<  " Sending PONG to Client(" << client_connection->socket->remote_endpoint().port() << ")";
        client_connection->socket->send(boost::asio::buffer(client_connection->tx_buffer));

        LOG_DEBUG << function_id <<  " Setting Async Rx Callback for Client(" << client_connection->socket->remote_endpoint().port() << ")";
        client_connection->socket->async_receive(boost::asio::buffer(client_connection->rx_buffer), [=](const boost::system::error_code& ec, size_t bytes){rx_callback(ec, bytes, client_connection);});

        io.run();
    }

}

}


