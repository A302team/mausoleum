#pragma once
#include "common/IServer.h"
#include "common/logging/Logger.h"  
#include "common/Platform.h"
#include "VoiceClientManager.h"
#include "VoicePacketRouter.h"
#include "VoicePacketType.h"

class VoiceServer : public IServer{
private:
    VoiceClientManager clientManager;
    VoicePacketRouter packetRouter;
    std::chrono::steady_clock::time_point lastCleanupTime;
    const int CLEANUP_INTERVAL_SECONDS = 60; // 1분마다 타임아

public:
    VoiceServer() : lastCleanupTime(std::chrono::steady_clock::now()) {
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
    void handleJoin(ParsedPacket& packet);
    void handleVoiceData(ParsedPacket& packet);
    void handleLeave(ParsedPacket& packet);
};