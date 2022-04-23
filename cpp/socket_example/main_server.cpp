#include <iostream>
#include <thread>
#include <future>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "logger.hpp"
#include "server.hpp"
#include "client.hpp"

using namespace socket_example;



typedef struct
{
    std::string ip;
    uint16_t server_port;
    uint16_t client_port;
    uint16_t clients_number;
} EnvConfig;

EnvConfig configurations = 
{
    // Default values if config file is not present
    "127.0.0.1",
    31490,
    31400,
    1
};

const std::string configFileName("AddressTest_config.json");

void loadConfigurations()
{
    std::string function_id = getFunctionId(__func__);

    LOG_DEBUG << function_id << " Loading config file: " << configFileName.c_str();
    boost::property_tree::ptree root;

    try
    {
        boost::property_tree::read_json(configFileName.c_str(), root);
        configurations.ip = root.get<std::string>("ip");
        configurations.server_port = root.get<uint16_t>("server_port");
        configurations.client_port = root.get<uint16_t>("client_port");
        configurations.clients_number = root.get<uint16_t>("clients_number");
    }
    catch(const std::exception& e)
    {
        LOG_DEBUG << function_id << " Failed to load config file(" << configFileName.c_str() << "). Will use default configs.";
    }

    LOG_DEBUG << function_id << " Ip: " << configurations.ip.c_str() 
                       << ", server_port: " << configurations.server_port 
                       << ", client_port: " << configurations.client_port;

}


// ############# MAIN #############


int main(int argc, char *argv[])
{
    std::string function_id = getFunctionId(__func__);
    
    LOG_DEBUG << function_id <<  " MAIN start";

    loadConfigurations();
    std::string ip = configurations.ip;
    uint16_t server_port = configurations.server_port;
    uint16_t client_port = configurations.client_port;
    uint16_t numberOfClients = configurations.clients_number;

    Server server(ip, server_port);
    LOG_DEBUG << function_id <<  " Launching server thread";
    server.start();

  
    
    LOG_DEBUG << function_id <<  " Will idle on main loop. CTRL + C to stop";
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        LOG_DEBUG << function_id <<  " Idle on main loop. CTRL + C to stop";
        
        bool server_status = server.status() != std::future_status::ready;
        LOG_DEBUG << function_id <<  " Child threads status: server(" 
                  << (server_status ? "\033[1;32m RUNNING \033[0m" : "\033[1;31m STOPPED \033[0m") << ")";

        if(!server_status)
        {
            LOG_DEBUG << function_id <<  " All threads are STOPPED. Will exit main loop.";
            break;
        }
    }

    LOG_DEBUG << function_id <<  " MAIN end";

    return 0;
}
