#include <thread>
#include "LobbyServer.h"
#include "voice/VoiceServer.h"
#include "common/Logger.h"

int main()
{
    Logger::Init("server.log");

    // 로비 서버 스레드 (포트 9001)
    std::thread lobbyThread([]() {
        LobbyServer lobbyServer;
        lobbyServer.run(8001);
    });

    // 보이스 서버 스레드 (포트 9100)
    std::thread voiceThread([]() {
        VoiceServer voiceServer;
        voiceServer.run(8100);
    });

    lobbyThread.join();
    voiceThread.join();

    Logger::Shutdown();
    return 0;
}