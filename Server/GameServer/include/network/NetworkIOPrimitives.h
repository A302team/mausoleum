#pragma once
#include "common/Platform.h"

struct NetworkUdpEndpointConfig {
    int port = 0;
    int maxPacketSize = 2048;
};

struct NetworkTcpListenerConfig {
    int port = 0;
};

struct NetworkUdpEndpoint {
    SocketType sock = INVALID_SOCK;
    int port = 0;
    int maxPacketSize = 2048;
};

struct NetworkTcpListener {
    SocketType sock = INVALID_SOCK;
    int port = 0;
};
