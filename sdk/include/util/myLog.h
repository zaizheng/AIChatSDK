#pragma once
#include <mutex>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

namespace LogUtil{
class Logger{
    public:
        static void initLogger(const std::string& loggerName, const std::string& loggerFile, spdlog::level::level_enum logLevel = spdlog::level::info);
        static std::shared_ptr<spdlog::logger> getLogger();
    private:
        Logger();
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
    private:
        static std::shared_ptr<spdlog::logger> _logger;
        static std::mutex _mutex;
};

#define TRACE(format, ...) LogUtil::Logger::getLogger()->trace(std::string("[{:>10s}:{:<4d}]")+format, __FILE__, __LINE__, ##__VA_ARGS__)
#define DBG(format, ...) LogUtil::Logger::getLogger()->debug(std::string("[{:>10s}:{:<4d}]")+format, __FILE__, __LINE__, ##__VA_ARGS__)
#define INFO(format, ...) LogUtil::Logger::getLogger()->info(std::string("[{:>10s}:{:<4d}]")+format, __FILE__, __LINE__, ##__VA_ARGS__)
#define WARN(format, ...) LogUtil::Logger::getLogger()->warn(std::string("[{:>10s}:{:<4d}]")+format, __FILE__, __LINE__, ##__VA_ARGS__)
#define ERR(format, ...) LogUtil::Logger::getLogger()->error(std::string("[{:>10s}:{:<4d}]")+format, __FILE__, __LINE__, ##__VA_ARGS__)
#define CRIT(format, ...) LogUtil::Logger::getLogger()->critical(std::string("[{:>10s}:{:<4d}]")+format, __FILE__, __LINE__, ##__VA_ARGS__)

}