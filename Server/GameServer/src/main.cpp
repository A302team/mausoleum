#include <thread>
#include "LobbyServer.h"
#include "VoiceServer.h"

int main()
{
    // 로비 서버 스레드 (포트 9001)
    std::thread lobbyThread([]() {
        LobbyServer lobbyServer;
        lobbyServer.run(9001);
    });

    // 보이스 서버 스레드 (포트 9100)
    std::thread voiceThread([]() {
        VoiceServer voiceServer;
        voiceServer.run(9100);
    });

    lobbyThread.join();
    voiceThread.join();

    return 0;
}