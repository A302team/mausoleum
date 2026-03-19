#pragma once

#include "CoreMinimal.h"

/**
 * ====================================================================
 * Voice Chat AOP-Style Performance Profiling
 * ====================================================================
 * 
 * [사용법]
 * 비즈니스 로직을 수정하지 않고, 함수 최상단에 매크로 한 줄만 추가:
 * 
 *   void MyFunction() {
 *       VOICE_PROFILE_SCOPE(TEXT("MyFunction"));   // ← 이 한 줄만 추가
 *       ... 기존 로직 그대로 ...
 *   }
 * 
 * [출력 예시]
 *   [VoiceProfile] Encode: 0.45ms
 *   [VoiceProfile] Decode: 0.32ms
 * 
 * [릴리즈 빌드에서 제거]
 *   VOICE_PROFILING_ENABLED를 0으로 변경하면 모든 프로파일링 코드가
 *   컴파일 시점에 완전히 제거됩니다 (Zero-cost abstraction).
 * ====================================================================
 */

// ===== 프로파일링 ON/OFF 스위치 =====
#ifndef VOICE_PROFILING_ENABLED
#define VOICE_PROFILING_ENABLED 1
#endif

// ===== 전용 로그 카테고리 =====
DECLARE_LOG_CATEGORY_EXTERN(LogVoiceProfile, Log, All);  // AOP 프로파일링 전용
DECLARE_LOG_CATEGORY_EXTERN(LogVoiceChat, Log, All);     // Voice 모듈 일반 로그


/**
 * RAII 스코프 타이머 (AOP Cross-cutting Concern 핵심)
 * - 생성자에서 시작 시각 기록
 * - 소멸자에서 경과 시간 로깅
 * - 스코프를 나가면 자동 호출 → 비즈니스 로직 수정 불필요
 */
struct FVoiceScopeTimer
{
    FString ScopeName;
    double StartTime;

    FVoiceScopeTimer(const TCHAR* InName)
        : ScopeName(InName)
        , StartTime(FPlatformTime::Seconds())
    {
    }

    ~FVoiceScopeTimer()
    {
        const double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

        // 0.1ms 이상인 경우만 로깅 (노이즈 방지)
        if (ElapsedMs >= 0.1)
        {
            UE_LOG(LogVoiceProfile, Log, TEXT("[VoiceProfile] %s: %.2fms"), *ScopeName, ElapsedMs);
        }
    }
};

// ===== AOP 매크로 (비즈니스 로직에 삽입하는 유일한 인터페이스) =====
#if VOICE_PROFILING_ENABLED

    // 범용 스코프 프로파일러 (이름 직접 지정)
    #define VOICE_PROFILE_SCOPE(Name)       FVoiceScopeTimer _VoiceTimer_##__LINE__(Name)

    // 파이프라인 구간별 전용 매크로
    #define VOICE_PROFILE_CAPTURE()         VOICE_PROFILE_SCOPE(TEXT("Capture"))
    #define VOICE_PROFILE_ENCODE()          VOICE_PROFILE_SCOPE(TEXT("Encode"))
    #define VOICE_PROFILE_DECODE()          VOICE_PROFILE_SCOPE(TEXT("Decode"))
    #define VOICE_PROFILE_NET_SEND()        VOICE_PROFILE_SCOPE(TEXT("NetSend"))
    #define VOICE_PROFILE_NET_RECV()        VOICE_PROFILE_SCOPE(TEXT("NetRecv"))
    #define VOICE_PROFILE_PLAYBACK()        VOICE_PROFILE_SCOPE(TEXT("Playback"))

#else

    // 프로파일링 비활성화 시 Zero-cost (컴파일러가 완전 제거)
    #define VOICE_PROFILE_SCOPE(Name)       do {} while(0)
    #define VOICE_PROFILE_CAPTURE()         do {} while(0)
    #define VOICE_PROFILE_ENCODE()          do {} while(0)
    #define VOICE_PROFILE_DECODE()          do {} while(0)
    #define VOICE_PROFILE_NET_SEND()        do {} while(0)
    #define VOICE_PROFILE_NET_RECV()        do {} while(0)
    #define VOICE_PROFILE_PLAYBACK()        do {} while(0)

#endif
