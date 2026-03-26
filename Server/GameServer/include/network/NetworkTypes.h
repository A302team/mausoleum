#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
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
    std::shared_ptr<const std::vector<char>> sharedData;

    const char* payloadData() const {
        if (sharedData) {
            return sharedData->empty() ? nullptr : sharedData->data();
        }
        return data.empty() ? nullptr : data.data();
    }

    size_t payloadSize() const {
        return sharedData ? sharedData->size() : data.size();
    }
};
