#ifdef _WIN32
#include "network/platform/SocketPlatform.h"
#include "network/platform/NetworkEventLoopWin.h"

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
            WSASetLastError(err);
            return INVALID_SOCK;
        }

        if (!setNonBlocking(sock)) {
            const int err = lastError();
            closeSocket(sock);
            WSASetLastError(err);
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
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
        if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCK_ERROR) {
            const int err = lastError();
            closeSocket(sock);
            WSASetLastError(err);
            return INVALID_SOCK;
        }

        if (listen(sock, SOMAXCONN) == SOCK_ERROR) {
            const int err = lastError();
            closeSocket(sock);
            WSASetLastError(err);
            return INVALID_SOCK;
        }

        if (!setNonBlocking(sock)) {
            const int err = lastError();
            closeSocket(sock);
            WSASetLastError(err);
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

std::unique_ptr<INetworkEventLoop> CreateNetworkEventLoop() {
    return std::make_unique<WindowsNetworkEventLoop>();
}
#endif
