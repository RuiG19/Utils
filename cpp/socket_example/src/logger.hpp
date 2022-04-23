#pragma once

#include <iostream>
#include <sstream>
#include <ostream>
#include <streambuf>
#include <thread>
#include <mutex>
#include <chrono>
#include <iomanip>

namespace socket_example
{

std::string getFunctionId(const std::string& function_name, const std::string& class_name = std::string());

enum class LogLevel : uint8_t
{
    // NONE = 0,
    // FATAL = 1,
    ERROR = 2,
    WARNING = 3,
    // INFO = 4,
    DEBUG = 5,
    // VERBOSE = 6
};

class Logger : public std::ostream
{
public:
Logger(LogLevel level);
~Logger();

private:
    class buffer : public std::streambuf {
    public:
        int_type overflow(int_type);
        std::streamsize xsputn(const char *, std::streamsize);

        std::stringstream data_;
    };

LogLevel level_;
buffer buffer_;
std::chrono::system_clock::time_point when_;
static std::mutex mutex__;

};

#define LOG_DEBUG   socket_example::Logger(socket_example::LogLevel::DEBUG)
#define LOG_WARNING socket_example::Logger(socket_example::LogLevel::WARNING)
#define LOG_ERROR   socket_example::Logger(socket_example::LogLevel::ERROR)


}
