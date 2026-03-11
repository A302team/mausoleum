#pragma once
#include "Platform.h"
#include "logging/Logger.h"

/**
 * @brief 서버 인터페이스
 * 
 */
class IServer {
public:
    virtual ~IServer() = default;
    virtual void run(int port){

        // Windows 소켓 초기화
#ifdef _WIN32
        WSADATA wsaData;
        if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
            LOG_ERROR(tag(), "WSAStartup failed");
            return;
        }
#endif
        // 소켓 초기화
        if(!initSocket(port)){
#ifdef _WIN32
            WSACleanup();
#endif
            return;
        }

        LOG_INFO(tag(), "서버 시작! 포트:" << port);
        onStarted(port); // 서버 시작 후 추가 초기화가 필요한 경우 오버라이드
        runLoop(); // 서버 메인 루프

        CLOSE_SOCKET(sock);
#ifdef _WIN32
        WSACleanup();
#endif
    }
    virtual void shutdown() {running = false;}

protected:
    SocketType sock = INVALID_SOCK;
    bool running = true;

    virtual const char* tag() const = 0; // 로그 태그
    virtual bool initSocket(int port) = 0;
    virtual void onStarted(int port) {}
    virtual void runLoop() = 0;

};