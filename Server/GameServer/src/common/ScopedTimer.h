#pragma once

#include <chrono>
#include <string>
#include "logging/Logger.h"

/**
 * @brief RAII 스코프 프로파일러
 * @details
 * - 특정 코드 블록의 실행 시간을 측정하여 로그로 출력
 * - 사용 예시:
 *   LOG_INFO("MyTag", "This is an info message");
 */
class ScopedTimer
{
public:
    /**
     * @brief 생성자
     * @param name 프로파일러 이름
     */
    ScopedTimer(const std::string& name)
        : name(name)
    {
        start = std::chrono::high_resolution_clock::now();
    }

    /**
     * @brief 소멸자
     * @details
     * - 코드 블록의 실행 시간을 측정하여 로그로 출력
     */
    ~ScopedTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();

        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        Logger::Log(Logger::Level::Info, "PROFILE",
            name + " took " + std::to_string(duration) + " us");
    }

private:

    std::string name;
    std::chrono::high_resolution_clock::time_point start;
};

#define PROFILE_SCOPE(name) ScopedTimer _timer_##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(FUNC_SIG)
