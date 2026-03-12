#pragma once
#include <algorithm>
#include <thread>
#include <vector>
#include "common/IServer.h"
#include "common/logging/Logger.h"  
#include "common/Platform.h"
#include "common/ConcurrentQueue.h"
#include "voice/VoiceClientManager.h"
#include "voice/VoicePacketRouter.h"
#include "voice/VoicePacketType.h"

class VoiceServer : public IServer{
private:
    struct PacketJob {
        std::vector<char> data;
        int size = 0;
        sockaddr_in senderAddr{};
        CLIENT_KEY senderKey = 0;
    };

    VoiceClientManager clientManager;
    VoicePacketRouter packetRouter;
    ConcurrentQueue<PacketJob> jobQueue;
    std::vector<std::thread> workers;
    size_t workerCount = 2;
    std::chrono::steady_clock::time_point lastCleanupTime;
    const int CLEANUP_INTERVAL_SECONDS = 60; // 1분마다 타임아
    static constexpr int MAX_PACKET_SIZE = 1024;

public:
    VoiceServer() : lastCleanupTime(std::chrono::steady_clock::now()) {
        unsigned int hc = std::thread::hardware_concurrency();
        if (hc == 0) {
            workerCount = 2;
        } else {
            workerCount = std::min(4u, std::max(2u, hc));
        }
        // 패킷 핸들러 등록
        packetRouter.registerHandler(VoicePacketType::Join, [this](ParsedPacket& packet){
            handleJoin(packet);
        });
        packetRouter.registerHandler(VoicePacketType::VoiceData, [this](ParsedPacket& packet){
            handleVoiceData(packet);
        });
        packetRouter.registerHandler(VoicePacketType::Leave, [this](ParsedPacket& packet){
            handleLeave(packet);\
            
        });
    }
protected:
    const char* tag() const override { return "VoiceServer"; }
    bool initSocket(int port) override;
    void runLoop() override;
private:
    void workerLoop();
    bool buildPacket(const PacketJob& job, ParsedPacket& outPacket);
    void handleJoin(ParsedPacket& packet);
    void handleVoiceData(ParsedPacket& packet);
    void handleLeave(ParsedPacket& packet);
};
