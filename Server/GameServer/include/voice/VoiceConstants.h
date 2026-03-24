#pragma once
#include <cstddef>
#include <cstdint>

namespace Voice::Protocol {
    constexpr size_t ROOM_CODE_SIZE = 16;
    constexpr size_t SPEAKER_NAME_SIZE = 32;
}

namespace Voice::Config {
    constexpr int CLEANUP_INTERVAL_SECONDS = 60;
    constexpr int CLIENT_TIMEOUT_SECONDS = 300;
    constexpr int UDP_MAX_PACKET_SIZE = 4096;
    constexpr uint32_t LOG_PAYLOAD_MIN_SIZE = 2;
}
