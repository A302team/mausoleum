#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include "Logger.h"

#pragma comment(lib, "ws2_32.lib")

class IServer {
public:
    virtual ~IServer() = default;
    virtual void run(int port){
        WSADATA wsaData;
        if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
            LOG_ERROR(tag(), "WSAStartup failed");
            return;
        }

        if(!initSocket(port)){
            WSACleanup();
            return;
        }

        LOG_INFO(tag(), "서버 시작! 포트:" << port);
        onStarted(port);
        runLoop();

        closesocket(sock);
        WSACleanup();
    }
    virtual void shutdown() {running = false;}

protected:
    SOCKET sock = INVALID_SOCKET;
    bool running = true;

    virtual const char* tag() const = 0;
    virtual bool initSocket(int port) = 0;
    virtual void onStarted(int port) {}
    virtual void runLoop() = 0;

};