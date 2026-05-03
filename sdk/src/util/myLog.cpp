#include "../../include/util/myLog.h"
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>

namespace LogUtil{
    std::shared_ptr<spdlog::logger> Logger::_logger = nullptr;
    std::mutex Logger::_mutex;

    Logger::Logger()
    {}

    void Logger::initLogger(const std::string& loggerName, const std::string& loggerFile, spdlog::level::level_enum logLevel){
        if(nullptr == _logger){
            std::lock_guard<std::mutex> lock(_mutex);
            if(nullptr == _logger){
                spdlog::flush_on(logLevel);
                spdlog::init_thread_pool(32768, 1);
                if("stdout" == loggerFile){
                    _logger = spdlog::stdout_color_mt(loggerName);
                }else{
                    _logger = spdlog::basic_logger_mt<spdlog::async_factory>(loggerName, loggerFile);
                }
            }
            _logger->set_pattern("[%H:%M:%S][%n][%-7l]%v");
            _logger->set_level(logLevel);
        }
    }

    std::shared_ptr<spdlog::logger> Logger::getLogger(){
        return _logger;
    }

}