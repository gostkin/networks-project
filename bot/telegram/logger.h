#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <chrono>
#include <ctime>
#include <memory>

namespace logger {

enum class LogLevel { Info = 0, Error = 1 };

inline std::string to_string(LogLevel level) {
    if (level == LogLevel::Info) {
        return "INFO";
    } else if (level == LogLevel::Error) {
        return "ERROR";
    }
    throw std::runtime_error("invalid log level");
}

class Logger {
public:
    Logger(LogLevel min_level) : min_level_{min_level} {
    }
    void LogInfo(const std::string& message) {
        Log(LogLevel::Info, message);
    }
    void LogError(const std::string& message) {
        Log(LogLevel::Error, message);
    }

    virtual ~Logger() {
    }

protected:
    virtual void Log(LogLevel cur_level, const std::string& message) = 0;

protected:
    LogLevel min_level_;
};

class OstreamLogger : public Logger {
public:
    OstreamLogger(std::ostream& out) : Logger(LogLevel::Info), out_{out} {
    }

protected:
    void Log(LogLevel cur_level, const std::string& message) override final {
        if (cur_level < min_level_) {
            return;
        }
        auto now = std::chrono::system_clock::now();
        auto old_time = std::chrono::system_clock::to_time_t(now);
        try {
            out_ << to_string(cur_level) << "\t" << message << "\t" << ctime(&old_time);
        } catch (...) {
            std::cerr << "Logger error...";
        }
    }

private:
    std::ostream& out_;
    std::string context_;
};

class LoggerFactory {
public:
    static std::shared_ptr<Logger> GetStdoutLogger() {
        static std::shared_ptr<Logger> logger =
            std::dynamic_pointer_cast<Logger>(std::make_shared<OstreamLogger>(std::cout));
        return logger;
    }
};

}  // namespace logger

#endif  // LOGGER_H
