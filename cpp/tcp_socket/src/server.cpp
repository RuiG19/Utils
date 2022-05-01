#include "server.hpp"
#include "logger.hpp"

namespace tcp
{

Server::Server(std::string ip_, uint16_t port_, std::function<void(uint16_t clientPort, std::unique_ptr<Payload> rxBuffer_)> handler_) : handler(handler_)
{
    server_endpoint = Endpoint(boost::asio::ip::make_address_v4(ip_), port_);
}

Server::~Server()
{
    std::string function_id = getFunctionId(__func__, "Server");

    connections.clear();

    try
    {
        acceptor->cancel();
        acceptor->close();

        io.stop();
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

void Server::send(uint16_t clientPort, std::unique_ptr<Payload> txBuffer_)
{
    std::string function_id = getFunctionId(__func__, "Server");

    if(txBuffer_->size() == 0)
    {
        LOG_WARNING << function_id <<  " Empty Payload, will ignore!";
        return;
    }

    if(connections.find(clientPort) != connections.end())
    {
        LOG_DEBUG << function_id <<  " Sending Payload with " << txBuffer_->size() << " bytes to Client(" << clientPort << ")";
        connections.at(clientPort)->getTxBuffer() = *txBuffer_;
        connections.at(clientPort)->getSocket().send(boost::asio::buffer(connections.at(clientPort)->getTxBuffer()));
    }
    else
    {
        LOG_WARNING << function_id <<  " No Connection available to client with port " << clientPort;   
    }
}

void Server::start_up()
{
    std::string function_id = getFunctionId(__func__, "Server");

    LOG_DEBUG << function_id <<  " Starting SERVER thread";
    acceptor = std::make_unique<Acceptor>(io);

    try
    {
        LOG_DEBUG << function_id <<  " OPEN ip_v4 socket";
        acceptor->open(server_endpoint.protocol().v4());

        LOG_DEBUG << function_id <<  " SET_OPTION reuse_address(true)";
        acceptor->set_option(Acceptor::reuse_address(true));
        
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
                
                connection->getTxBuffer() = {'P','O','N','G'}; 
                connection->getRxBuffer() = Payload(4090); 
                
                acceptor->accept(connection->getSocket());

                LOG_DEBUG << function_id <<  " New connection accepted with Client(" << connection->getSocket().remote_endpoint().port() << ")";
                {
                    std::lock_guard<std::mutex> lock(rx_mutex);
                    connections.emplace(connection->getSocket().remote_endpoint().port(), connection);
                }

                LOG_DEBUG << function_id <<  " Setting Async Rx Callback for Client(" << connection->getSocket().remote_endpoint().port() << ")";
                connection->getSocket().async_receive(boost::asio::buffer(connection->getRxBuffer()), 
                    [=](const boost::system::error_code& ec, size_t bytes)
                    {
                        rx_callback(ec, bytes, connection);
                    });
                
                connection->start();

                // LOG_DEBUG << function_id <<  " Starting io_context run";
                // io.run();
                
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
    uint16_t clientPort = client_connection->getSocket().remote_endpoint().port();
    LOG_DEBUG << function_id <<  " Got something from Client(" << clientPort << ")";
    
    if(ec)
    {
        LOG_WARNING << function_id <<  " Erro code: " << ec.message();
        if(ec == boost::asio::error::eof)
        {
            LOG_DEBUG << function_id <<  " Client(" << clientPort << ") closed the connection!";
            connections.erase(clientPort);
        }
        return;
    }

    LOG_DEBUG << function_id <<  " Received " << bytes << " bytes";
    if(bytes)
    {
        std::string payload(client_connection->getRxBuffer().begin(), client_connection->getRxBuffer().end());
        LOG_DEBUG << function_id <<  " Rx Payload: " << payload;

        if(handler)
        {
            std::unique_ptr<Payload> rxPayload = std::make_unique<Payload>(client_connection->getRxBuffer());
            handler(clientPort, std::move(rxPayload));
        }
        else
        {
            // if no hadnler is defined simply Pong the client (use as default impl - maybe be comment out this section later)
            LOG_DEBUG << function_id <<  " Sending PONG to Client(" << clientPort << ")";
            client_connection->getSocket().send(boost::asio::buffer(client_connection->getTxBuffer()));
        }

        LOG_DEBUG << function_id <<  " Setting Async Rx Callback for Client(" << clientPort << ")";
        client_connection->getSocket().async_receive(boost::asio::buffer(client_connection->getRxBuffer()), 
            [=](const boost::system::error_code& ec, size_t bytes)
            {
                rx_callback(ec, bytes, client_connection);
            });
    }

}


Server::Connection::Connection()
    : socket(std::make_shared<Socket>(context_io))
{

}

Server::Connection::~Connection()
{
    std::string function_id = getFunctionId(__func__, "Server");

    rx_buffer.resize(0);
    tx_buffer.resize(0);

    try
    {
        uint16_t port = socket->remote_endpoint().port();
        LOG_DEBUG << function_id <<  " Deleting endpoint: " << port;
        socket->cancel();
        socket->close();
        
        context_io.stop();
        LOG_DEBUG << function_id <<  " Endpoint(" << port << ") context_io stopped: " << std::boolalpha << context_io.stopped();
        _thread.detach(); // cannot use join since this is most likely being executted in the same context (Server::rx_callback() uses this thread context)
        LOG_DEBUG << function_id <<  " Endpoint(" << port << ") thread detached";
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << function_id << e.what();
    }
}

void Server::Connection::start()
{
    _thread = std::thread([=](){ context_io.run(); });
}

Socket& Server::Connection::getSocket()
{
    return *socket;
}

Payload& Server::Connection::getRxBuffer()
{
    return rx_buffer;
}

Payload& Server::Connection::getTxBuffer()
{
    return tx_buffer;
}



}


