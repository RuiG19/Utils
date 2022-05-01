#pragma once

#include <iostream>
#include <sstream>
#include <ostream>
#include <streambuf>
#include <thread>
#include <mutex>
#include <chrono>
#include <iomanip>

namespace tcp
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

static void setMaximumLogLevel(LogLevel level);

private:
    class buffer : public std::streambuf {
    public:
        int_type overflow(int_type);
        std::streamsize xsputn(const char *, std::streamsize);

        std::stringstream data_;
    };

LogLevel level_;
static LogLevel maxLevel_;
buffer buffer_;
std::chrono::system_clock::time_point when_;
static std::mutex mutex__;

};

#define LOG_DEBUG   tcp::Logger(tcp::LogLevel::DEBUG)
#define LOG_WARNING tcp::Logger(tcp::LogLevel::WARNING)
#define LOG_ERROR   tcp::Logger(tcp::LogLevel::ERROR)


}
