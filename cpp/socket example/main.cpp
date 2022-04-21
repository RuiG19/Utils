#include <iostream>
#include <thread>
#include <future>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>


// ############# UTILS #############
#define USE_COUT_COLORS

typedef struct
{
    std::string ip;
    uint16_t server_port;
    uint16_t client_port;
} EnvConfig;

EnvConfig configurations = 
{
    // Default values if config file is not present
    "127.0.0.1",
    31490,
    31400
};

const std::string configFileName("AddressTest_config.json");

// ############# UTILS #############

// to NOT run server_rx and client_rx at the same time, so we dont mess the logs
std::mutex rx_mutex;

std::string getTimestamp()
{
    // FIX ME - Time is not updating after the first call
    // std::time_t currentTime = std::time(NULL);
    // struct tm * timeStruct = std::localtime(&currentTime);
    // std::stringstream time_stream;
    // time_stream << "[" << std::setw(2) << timeStruct->tm_hour 
    //             << ":" << std::setw(2) << timeStruct->tm_min 
    //             << ":" << std::setw(2) << timeStruct->tm_sec << "] ";
    // return time_stream.str();
    return "";
}

std::string getFunctionId(const std::string& function_name)
{
    std::stringstream log_id;
    uint16_t thread_id = (uint16_t)std::hash<std::thread::id>{}(std::this_thread::get_id());

#ifdef USE_COUT_COLORS
    log_id << getTimestamp() << "\033[1;31m" << function_name << "(" << (int)thread_id << ")" << "\033[0m";
#elif
    log_id << getTimestamp() << function_name << "(" << (int)thread_id << ")";
#endif
    return log_id.str();
}

void loadConfigurations()
{
    std::string function_id = getFunctionId(__func__);

    std::cout << function_id << " Loading config file: " << configFileName.c_str() << std::endl;
    boost::property_tree::ptree root;

    try
    {
        boost::property_tree::read_json(configFileName.c_str(), root);
        configurations.ip = root.get<std::string>("ip");
        configurations.server_port = root.get<uint16_t>("server_port");
        configurations.client_port = root.get<uint16_t>("client_port");
    }
    catch(const std::exception& e)
    {
        std::cerr << function_id << " Failed to load config file(" << configFileName.c_str() << "). Will use default configs." << std::endl;
    }

    std::cout << function_id << " Ip: " << configurations.ip.c_str() 
                             << ", server_port: " << configurations.server_port 
                             << ", client_port: " << configurations.client_port 
                             << std::endl;

}

// ############# SERVER #############
std::vector<char> server_rx_buffer(4090); 
std::vector<char> server_tx_buffer = {'P','O','N','G'}; 

void server_rx_f(const boost::system::error_code& ec, size_t bytes )
{
    // lock to NOT run server_rx and client_rx at the same time, so we dont mess the logs
    std::lock_guard<std::mutex> lock(rx_mutex);

    std::string function_id = getFunctionId(__func__);
    std::cout << function_id <<  " Got something!" << std::endl;
    
    if(ec)
    {
        std::cerr << function_id <<  " Erro code: " << ec.message() << std::endl;
        return;
    }

    if(bytes)
    {
        std::cout << function_id <<  " Received " << bytes << " bytes" << std::endl;
        std::string payload(server_rx_buffer.begin(), server_rx_buffer.end());
        std::cout << function_id <<  " Rx Payload: " << payload << std::endl;
    }
}

void server_accept_f(const boost::system::error_code& ec, std::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    std::string function_id = getFunctionId(__func__);
    
    if(ec)
    {
        std::cerr << function_id <<  " Erro code: " << ec.message() << std::endl;
        return;
    }

    std::cout << function_id <<  " New connection accepted" << std::endl;

    std::cout << function_id <<  " Setting Async Rx Callback" << std::endl;
    socket->async_receive(boost::asio::buffer(server_rx_buffer), server_rx_f);

    std::cout << function_id <<  " Sending PONG to Client(" << socket->remote_endpoint().port() << ")" << std::endl;
    socket->send(boost::asio::buffer(server_tx_buffer));
}

void server_f(const boost::asio::ip::tcp::endpoint& endpoint)
{
    std::string function_id = getFunctionId(__func__);

    std::cout << function_id <<  " Starting SERVER thread" << std::endl;
    boost::asio::io_context io;
    boost::asio::ip::tcp::acceptor acceptor(io);

    try
    {
        std::cout << function_id <<  " OPEN ip_v4 socket" << std::endl;
        acceptor.open(endpoint.protocol().v4());

        std::cout << function_id <<  " SET_OPTION reuse_address(true)" << std::endl;
        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        
        std::cout << function_id <<  " BIND [" << endpoint.address().to_string() << ":" 
                              << endpoint.port() << "]" << std::endl;
        acceptor.bind(endpoint);
        
        std::cout << function_id <<  " LISTEN start" << std::endl;
        acceptor.listen(boost::asio::socket_base::max_connections);    
    }
    catch(const std::exception& e)
    {
        std::cerr << function_id << " " << e.what() << std::endl;
        exit (EXIT_FAILURE);
    }    

    if (acceptor.is_open())
    {
        std::cout << function_id <<  " ACCEPTOR is open ... start waiting for a new connections" << std::endl;
        std::shared_ptr<boost::asio::ip::tcp::socket> server_socket =
            std::make_shared<boost::asio::ip::tcp::socket>(acceptor.get_executor());

        acceptor.async_accept(*server_socket,
                              std::bind(
                                  server_accept_f,
                                  std::placeholders::_1, 
                                  server_socket
                              ));
    }
    else
    {
        // should never reach here as we catch this exception before, but we never know ¯\_(ツ)_/¯
        std::cerr << function_id << "acceptor.is_open() == false" << std::endl;
        exit (EXIT_FAILURE);
    }
    
    std::cout << function_id <<  " Starting io_context run" << std::endl;
    io.run();

}

// ############# CLIENT #############
std::vector<char> client_rx_buffer(4090); 
std::vector<char> client_tx_buffer = {'P','I','N','G'}; 

void client_rx_f(const boost::system::error_code& ec, size_t bytes )
{
    // lock to NOT run server_rx and client_rx at the same time, so we dont mess the logs
    std::lock_guard<std::mutex> lock(rx_mutex);

    std::string function_id = getFunctionId(__func__);
    std::cout << function_id <<  " Got something!" << std::endl;
    
    if(ec)
    {
        std::cerr << function_id <<  " Erro code: " << ec.message() << std::endl;
        return;
    }

    if(bytes)
    {
        std::cout << function_id <<  " Received " << bytes << " bytes" << std::endl;
        std::string payload(client_rx_buffer.begin(), client_rx_buffer.end());
        std::cout << function_id <<  " Rx Payload: " << payload << std::endl;
    }
}

void client_f(const boost::asio::ip::tcp::endpoint& client_endpoint, 
              const boost::asio::ip::tcp::endpoint& server_endpoint)
{
    std::string function_id = getFunctionId(__func__);

    std::cout << function_id << " Starting CLIENT thread" << std::endl;

    boost::asio::io_context io;
    std::shared_ptr<boost::asio::ip::tcp::socket> socket = std::make_shared<boost::asio::ip::tcp::socket>(io);

    try
    {
        std::cout << function_id <<  " OPEN ip_v4 socket" << std::endl;
        socket->open(client_endpoint.protocol().v4());

        std::cout << function_id <<  " SET_OPTION reuse_address(true)" << std::endl;
        socket->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

        std::cout << function_id <<  " BIND [" << client_endpoint.address().to_string() << ":" 
                              << client_endpoint.port() << "]" << std::endl;
        socket->bind(client_endpoint);

        std::cout << function_id <<  " CONNECT TO [" << server_endpoint.address().to_string() << ":" 
                              << server_endpoint.port() << "]" << std::endl;
        socket->connect(server_endpoint);

        std::cout << function_id <<  " Setting Async Rx Callback" << std::endl;
        socket->async_receive(boost::asio::buffer(client_rx_buffer), client_rx_f);

        std::cout << function_id <<  " Sending PING to Server(" << socket->remote_endpoint().port() << ")" << std::endl;
        socket->send(boost::asio::buffer(client_tx_buffer));
    }
    catch(const std::exception& e)
    {
        std::cerr << function_id << " " << e.what() << '\n';
        exit (EXIT_FAILURE);
    }
    
    std::cout << function_id <<  " Starting io_context run" << std::endl;
    io.run();
}

// ############# MAIN #############


int main(int argc, char *argv[])
{
    std::string function_id = getFunctionId(__func__);
    
    std::cout << function_id <<  " MAIN start" << std::endl;

    loadConfigurations();
    std::string ip = configurations.ip;
    uint16_t server_port = configurations.server_port;
    uint16_t client_port = configurations.client_port;

    boost::asio::ip::tcp::endpoint server_endpoint(boost::asio::ip::make_address_v4(ip), server_port);
    boost::asio::ip::tcp::endpoint client_endpoint(boost::asio::ip::make_address_v4(ip), client_port);

    std::cout << function_id <<  " Launching server thread" << std::endl;
    // std::thread server_thread(server_f, server_endpoint);
    auto server_future = std::async(std::launch::async, server_f, server_endpoint);

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // some time so the server can init
    
    std::cout << function_id <<  " Launching client thread" << std::endl;
    // std::thread client_thread(client_f, client_endpoint, server_endpoint);
    auto client_future = std::async(std::launch::async, client_f, client_endpoint, server_endpoint);

    std::cout << function_id <<  " Will idle on main loop. CTRL + C to stop" << std::endl;
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << function_id <<  " Idle on main loop. CTRL + C to stop" << std::endl;
        bool server_status = server_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready;
        bool client_status = client_future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready;
        std::cout << function_id <<  " Child threads status: server(" 
                                 << (server_status ? "RUNNING" : "STOPPED") 
                                 << "), client(" 
                                 << (client_status ? "RUNNING" : "STOPPED") 
                                 << ")" << std::endl;

        if(!server_status && !client_status)
        {
            std::cout << function_id <<  " All threads are STOPPED. Will exit main loop." << std::endl;
            break;
        }
    }

    std::cout << function_id <<  " MAIN end" << std::endl;

    return 0;
}
