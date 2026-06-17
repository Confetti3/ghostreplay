#include "util/Logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

Logger& Logger::instance()
{
    static Logger l;
    return l;
}

void Logger::setLogFile(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) m_file.close();
    m_file.open(path, std::ios::app);
}

void Logger::log(Level level, const std::string& message)
{
    if (level < m_min_level) return;

    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::lock_guard<std::mutex> lock(m_mutex);

    // Console output
    std::cerr << "[" << levelString(level) << "] "
              << std::put_time(std::localtime(&t), "%H:%M:%S")
              << "." << std::setfill('0') << std::setw(3) << ms.count()
              << " " << message << std::endl;

    // File output
    if (m_file.is_open())
    {
        m_file << "[" << levelString(level) << "] "
               << std::put_time(std::localtime(&t), "%H:%M:%S")
               << "." << std::setfill('0') << std::setw(3) << ms.count()
               << " " << message << std::endl;
        m_file.flush();
    }
}

const char* Logger::levelString(Level level)
{
    switch (level)
    {
    case Debug:   return "DEBUG";
    case Info:    return "INFO";
    case Warning: return "WARN";
    case Error:   return "ERROR";
    default:      return "????";
    }
}
