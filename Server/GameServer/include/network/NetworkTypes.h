#pragma once
#include <cstdint>
#include <vector>
#include "common/Platform.h"

enum class NetProtocol : uint8_t {
    Udp = 0,
    Tcp = 1
};

using ConnectionId = uint64_t;

struct NetPacket {
    NetProtocol protocol = NetProtocol::Udp;
    ConnectionId connectionId = 0; // TCP 연결 식별자 (UDP는 0)
    sockaddr_in addr{};
    std::vector<char> data;
};
