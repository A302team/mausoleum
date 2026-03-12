#ifndef _WIN32
#include "network/platform/SocketPlatform.h"

class PosixSocketPlatform final : public SocketPlatform {
public:
    bool initialize() override {
        return true;
    }

    void shutdown() override {}

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
#endif
