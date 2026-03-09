#include "Logger.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <windows.h>

std::ofstream Logger::logFile;

std::queue<Logger::LogMessage> Logger::queue;
std::mutex Logger::queueMutex;
std::condition_variable Logger::cv;

std::thread Logger::workerThread;
std::atomic<bool> Logger::running(false);

void Logger::Init(const std::string& file)
{
    logFile.open(file);

    running = true;

    workerThread = std::thread(Worker);
}

void Logger::Shutdown()
{
    running = false;

    cv.notify_all();

    if (workerThread.joinable())
        workerThread.join();

    logFile.close();
}

std::string Logger::GetTime()
{
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);

    std::tm tm;
    localtime_s(&tm, &t);

    std::ostringstream oss;

    oss << std::put_time(&tm,"%H:%M:%S");

    return oss.str();
}

const char* Logger::LevelToString(Level level)
{
    switch(level)
    {
        case Level::Info: return "INFO";
        case Level::Warn: return "WARN";
        case Level::Error: return "ERROR";
    }

    return "";
}

void Logger::Log(Level level,const std::string& tag,const std::string& msg)
{
    LogMessage m;

    m.level = level;
    m.tag = tag;
    m.msg = msg;
    m.threadId = std::this_thread::get_id();
    m.time = GetTime();

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.push(m);
    }

    cv.notify_one();
}

void Logger::Worker()
{
    while (running)
    {
        std::unique_lock<std::mutex> lock(queueMutex);

        cv.wait(lock, [] { return !queue.empty() || !running; });

        while (!queue.empty())
        {
            auto m = queue.front();
            queue.pop();

            lock.unlock();

            int color = 7;

            if (m.level == Level::Info) color = 10;
            if (m.level == Level::Warn) color = 14;
            if (m.level == Level::Error) color = 12;

            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);

            std::stringstream ss;

            ss << "[" << m.time << "] "
               << "[" << m.threadId << "] "
               << "[" << LevelToString(m.level) << "] "
               << "[" << m.tag << "] "
               << m.msg;

            std::cout << ss.str() << std::endl;

            if (logFile.is_open())
                logFile << ss.str() << std::endl;

            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);

            lock.lock();
        }
    }
}