#pragma once

#include <string>
#include <fstream>
#include <mutex>

// Minimal file logger. No dependency on spdlog for Phase 0.
class Logger
{
public:
    enum Level { Debug, Info, Warning, Error };

    static Logger& instance();

    void setLogFile(const std::string& path);
    void setLevel(Level min_level) { m_min_level = min_level; }

    void log(Level level, const std::string& message);

    void debug(const std::string& msg)   { log(Debug, msg); }
    void info(const std::string& msg)    { log(Info, msg); }
    void warning(const std::string& msg) { log(Warning, msg); }
    void error(const std::string& msg)   { log(Error, msg); }

private:
    Logger() = default;
    std::ofstream m_file;
    Level m_min_level = Info;
    std::mutex m_mutex;

    static const char* levelString(Level level);
};

// Convenience macros
#define LOG_DEBUG(msg)   Logger::instance().debug(msg)
#define LOG_INFO(msg)    Logger::instance().info(msg)
#define LOG_WARNING(msg) Logger::instance().warning(msg)
#define LOG_ERROR(msg)   Logger::instance().error(msg)
