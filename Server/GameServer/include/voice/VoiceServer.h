#pragma once
#include <chrono>
#include "common/IServer.h"
#include "common/logging/Logger.h"
#include "network/NetworkService.h"
#include "voice/VoiceClientManager.h"
#include "voice/VoiceConstants.h"
#include "voice/VoicePacketRouter.h"
#include "voice/VoicePacketType.h"

class VoiceServer : public IServer{
private:
    VoiceClientManager clientManager;
    VoicePacketRouter packetRouter;
    NetworkService network;
    std::chrono::steady_clock::time_point lastCleanupTime;

public:
    VoiceServer();
    void run(int port) override;
    void shutdown() override;
protected:
    const char* tag() const override { return "VoiceServer"; }
    bool initSocket(int port) override { return true; }
    void runLoop() override {}
private:
    void onUdpPacket(const NetPacket& packet);
    bool buildPacket(const NetPacket& packet, ParsedPacket& outPacket);
    void handleJoin(ParsedPacket& packet);
    void handleVoiceData(ParsedPacket& packet);
    void handleLeave(ParsedPacket& packet);
};
