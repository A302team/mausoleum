#include "common/logging/Logger.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * @brief Logger 
 * 
 */
std::ofstream Logger::logFile; // 로그 파일 스트림

std::queue<Logger::LogMessage> Logger::queue; // 로그 메시지 큐
std::mutex Logger::queueMutex; // 큐 접근 동기화
std::condition_variable Logger::cv; // 큐에 새 메시지가 추가될 때 알림

std::thread Logger::workerThread; // 로그 처리 스레드
std::atomic<bool> Logger::running(false); // 로그 처리 스레드 실행 여부

void Logger::Init(const std::string& file)
{
    // 없다면 새로 생성, 있다면 덮어쓰기
    logFile.open(file);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << file << std::endl;
        return;
    }

    running = true;
    workerThread = std::thread(Worker);
}

void Logger::Shutdown()
{
    running = false;

    cv.notify_all();

    if (workerThread.joinable())
        workerThread.join(); // 로그 처리 스레드 종료 대기

    logFile.close(); // 로그 파일 닫기
}

std::string Logger::GetTime()
{
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);

    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

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
        // 큐에서 로그 메시지를 가져와 처리
        std::unique_lock<std::mutex> lock(queueMutex); // 

        cv.wait(lock, [] { return !queue.empty() || !running; });

        while (!queue.empty())
        {
            auto m = queue.front();
            queue.pop();

            lock.unlock();

#ifdef _WIN32
            int color = 7;
            if (m.level == Level::Info) color = 10;
            if (m.level == Level::Warn) color = 14;
            if (m.level == Level::Error) color = 12;

            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#else
            const char* ansiColor = "\033[0m";
            if (m.level == Level::Info) ansiColor = "\033[32m";
            if (m.level == Level::Warn) ansiColor = "\033[33m";
            if (m.level == Level::Error) ansiColor = "\033[31m";

            std::cout << ansiColor;
#endif

            std::stringstream ss;

            ss << "[" << m.time << "] "
               << "[" << m.threadId << "] "
               << "[" << LevelToString(m.level) << "] "
               << "[" << m.tag << "] "
               << m.msg;

            std::cout << ss.str() << std::endl;

            if (logFile.is_open())
                logFile << ss.str() << std::endl;

#ifdef _WIN32
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
#else
            std::cout << "\033[0m"; // Reset terminal color
#endif

            lock.lock();
        }
    }
}
