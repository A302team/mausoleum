#ifdef _WIN32
#include "network/platform/SocketPlatform.h"
#include "common/logging/Logger.h"

class WindowsSocketPlatform final : public SocketPlatform {
public:
    bool initialize() override {
        if (initialized_) return true;
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return false;
        }
        initialized_ = true;
        return true;
    }

    void shutdown() override {
        if (!initialized_) return;
        WSACleanup();
        initialized_ = false;
    }

    bool setNonBlocking(SocketType sock) override {
        u_long mode = 1;
        return ioctlsocket(sock, FIONBIO, &mode) == 0;
    }

    void configureUdpSocket(SocketType sock) override {
        BOOL bFalse = FALSE;
        DWORD dwBytes = 0;
        WSAIoctl(sock, SIO_UDP_CONNRESET, &bFalse, sizeof(bFalse),
                 NULL, 0, &dwBytes, NULL, NULL);
    }

    int lastError() const override {
        return WSAGetLastError();
    }

private:
    bool initialized_ = false;
};

SocketPlatform& GetSocketPlatform() {
    static WindowsSocketPlatform instance;
    return instance;
}
#endif
