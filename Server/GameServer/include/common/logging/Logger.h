#pragma once

#include <string>
#include <sstream>
#include <chrono>
#include <fstream>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

/**
 * @brief 로깅 클래스
 * @details
 * - 비동기 로깅: 로그 메시지를 큐에 저장하고 별도의 스레드에서 처리
 * - 로그 레벨: Info, Warn, Error
 * - 로그 포맷: [시간] [스레드ID] [레벨] [태그] 메시지
 * - 콘솔과 파일에 동시에 출력
 * - 스코프 프로파일링 지원: 특정 코드 블록의 실행 시간을 측정하여 로그로 출력
 * - 사용 예시:
 *   LOG_INFO("MyTag", "This is an info message");
 */
class Logger
{
public:

    enum class Level
    {
        Info,
        Warn,
        Error
    };

    static void Init(const std::string& file);
    static void Shutdown();

    static void Log(Level level, const std::string& tag, const std::string& msg);

private:

    struct LogMessage
    {
        Level level;
        std::string tag;
        std::string msg;
        std::thread::id threadId;
        std::string time;
    };

    static std::string GetTime();
    static const char* LevelToString(Level level);
    static void Worker();

    // 로그 파일
    static std::ofstream logFile;

    // 로그 메시지 큐 및 동기화 도구
    static std::queue<LogMessage> queue;
    static std::mutex queueMutex; // 큐 접근 동기화
    static std::condition_variable cv; // 큐에 새 메시지가 추가될 때 알림

    static std::thread workerThread; // 로그 처리 스레드
    static std::atomic<bool> running; // 로그 처리 스레드 실행 여부
};

#define LOG_INFO(tag, msg)  do { std::ostringstream _ss; _ss << msg; Logger::Log(Logger::Level::Info, tag, _ss.str()); } while(0)
#define LOG_WARN(tag, msg)  do { std::ostringstream _ss; _ss << msg; Logger::Log(Logger::Level::Warn, tag, _ss.str()); } while(0)
#define LOG_ERROR(tag, msg) do { std::ostringstream _ss; _ss << msg; Logger::Log(Logger::Level::Error, tag, _ss.str()); } while(0)