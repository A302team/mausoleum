#ifndef _WIN32
#include "network/platform/SocketPlatform.h"
#if defined(__linux__)
#include "network/platform/NetworkEventLoopLinux.h"
#else
#include "network/platform/INetworkEventLoop.h"
#endif

class PosixSocketPlatform final : public SocketPlatform {
public:
    bool initialize() override {
        return true;
    }

    void shutdown() override {}

    SocketType createUdpEndpoint(int port) override {
        SocketType sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCK) {
            return INVALID_SOCK;
        }

        configureUdpSocket(sock);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCK_ERROR) {
            const int err = lastError();
            closeSocket(sock);
            errno = err;
            return INVALID_SOCK;
        }

        if (!setNonBlocking(sock)) {
            const int err = lastError();
            closeSocket(sock);
            errno = err;
            return INVALID_SOCK;
        }

        return sock;
    }

    SocketType createTcpListener(int port) override {
        SocketType sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCK) {
            return INVALID_SOCK;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(static_cast<uint16_t>(port));

        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCK_ERROR) {
            const int err = lastError();
            closeSocket(sock);
            errno = err;
            return INVALID_SOCK;
        }

        if (listen(sock, SOMAXCONN) == SOCK_ERROR) {
            const int err = lastError();
            closeSocket(sock);
            errno = err;
            return INVALID_SOCK;
        }

        if (!setNonBlocking(sock)) {
            const int err = lastError();
            closeSocket(sock);
            errno = err;
            return INVALID_SOCK;
        }

        return sock;
    }

    bool closeSocket(SocketType& sock) override {
        if (sock == INVALID_SOCK) {
            return true;
        }
        const int rc = CLOSE_SOCKET(sock);
        sock = INVALID_SOCK;
        return rc != SOCK_ERROR;
    }

    bool setNonBlocking(SocketType sock) override {
        int flags = fcntl(sock, F_GETFL, 0);
        if (flags == -1) return false;
        return fcntl(sock, F_SETFL, flags | O_NONBLOCK) != -1;
    }

    void configureUdpSocket(SocketType) override {}

    int lastError() const override {
        return errno;
    }
};

SocketPlatform& GetSocketPlatform() {
    static PosixSocketPlatform instance;
    return instance;
}

#if defined(__linux__)
std::unique_ptr<INetworkEventLoop> CreateNetworkEventLoop() {
    return std::make_unique<LinuxNetworkEventLoop>();
}
#else
std::unique_ptr<INetworkEventLoop> CreateNetworkEventLoop() {
    return nullptr;
}
#endif
#endif
