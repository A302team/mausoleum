#pragma once

#include <cstdint>

using CLIENT_KEY = uint64_t;

enum class  VoicePacketType : uint8_t {
    Join = 0,
    VoiceData = 1,
    Leave = 2
};

// std::unordered_map에서 enum class를 키로 사용하기 위한 해시 특수화
namespace std {
    template<> struct hash<VoicePacketType> { // 해시 함수 정의
        size_t operator()(VoicePacketType t) const noexcept {
            return hash<uint8_t>()(static_cast<uint8_t>(t));
        }
    };
}