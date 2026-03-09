#pragma once
#include "Platform.h"
#include "Logger.h"

class IServer {
public:
    virtual ~IServer() = default;
    virtual void run(int port){
#ifdef _WIN32
        WSADATA wsaData;
        if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
            LOG_ERROR(tag(), "WSAStartup failed");
            return;
        }
#endif

        if(!initSocket(port)){
#ifdef _WIN32
            WSACleanup();
#endif
            return;
        }

        LOG_INFO(tag(), "서버 시작! 포트:" << port);
        onStarted(port);
        runLoop();

        CLOSE_SOCKET(sock);
#ifdef _WIN32
        WSACleanup();
#endif
    }
    virtual void shutdown() {running = false;}

protected:
    SocketType sock = INVALID_SOCK;
    bool running = true;

    virtual const char* tag() const = 0;
    virtual bool initSocket(int port) = 0;
    virtual void onStarted(int port) {}
    virtual void runLoop() = 0;

};