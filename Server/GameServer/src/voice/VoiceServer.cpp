#include "voice/VoiceServer.h"
#include <thread>

namespace {
std::string fixedString(const char* buffer, size_t maxLen) {
    size_t len = 0;
    while (len < maxLen && buffer[len] != '\0') {
        ++len;
    }
    return std::string(buffer, len);
}
} // namespace

VoiceServer::VoiceServer() : lastCleanupTime(std::chrono::steady_clock::now()) {
    // 패킷 핸들러 등록
    packetRouter.registerHandler(VoicePacketType::Join, [this](ParsedPacket& packet){
        handleJoin(packet);
    });
    packetRouter.registerHandler(VoicePacketType::VoiceData, [this](ParsedPacket& packet){
        handleVoiceData(packet);
    });
    packetRouter.registerHandler(VoicePacketType::Leave, [this](ParsedPacket& packet){
        handleLeave(packet);
    });

    // 네트워크 수신 핸들러 등록
    network.registerHandler(NetProtocol::Udp, [this](const NetPacket& packet, NetworkService&){
        onUdpPacket(packet);
    });
}

void VoiceServer::run(int port) {
    running = true;

    if (!network.addUdpEndpoint(port, Voice::Config::UDP_MAX_PACKET_SIZE)) {
        LOG_ERROR(tag(), "UDP 엔드포인트 등록 실패: " << port);
        return;
    }
    if (!network.start()) {
        LOG_ERROR(tag(), "NetworkService 시작 실패");
        return;
    }

    while (running) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(
                now - lastCleanupTime).count() > Voice::Config::CLEANUP_INTERVAL_SECONDS) {
            clientManager.cleanupStaleClients();
            lastCleanupTime = now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    network.stop();
}

void VoiceServer::shutdown() {
    running = false;
    network.stop();
}

void VoiceServer::onUdpPacket(const NetPacket& packet) {
    if (packet.protocol != NetProtocol::Udp) {
        LOG_WARN(tag(), "UDP 핸들러에 TCP 패킷이 전달됨");
        return;
    }
    ParsedPacket parsed{};
    if (!buildPacket(packet, parsed)) {
        return;
    }
    packetRouter.dispatch(parsed);
}

bool VoiceServer::buildPacket(const NetPacket& packet, ParsedPacket& outPacket) {
    if (packet.data.size() < sizeof(VoicePacketHeader)) {
        LOG_WARN(tag(), "패킷 크기 부족: " << packet.data.size());
        return false;
    }

    auto* header = reinterpret_cast<const VoicePacketHeader*>(packet.data.data());
    const auto typeValue = static_cast<uint8_t>(header->packetType);
    if (typeValue > static_cast<uint8_t>(VoicePacketType::Leave)) {
        LOG_WARN(tag(), "알 수 없는 패킷 타입: " << static_cast<int>(typeValue));
        return false;
    }
    size_t payloadBytes = packet.data.size() - sizeof(VoicePacketHeader);
    if (header->packetType == VoicePacketType::VoiceData) {
        if (header->payloadSize > payloadBytes) {
            LOG_WARN(tag(), "payloadSize 불일치: " << header->payloadSize << " > " << payloadBytes);
            return false;
        }
    }

    outPacket = ParsedPacket{
        header,
        packet.data.data(),
        static_cast<int>(packet.data.size()),
        packet.addr,
        makeClientKey(packet.addr),
        fixedString(header->roomCode, sizeof(header->roomCode)),
        fixedString(header->speakerName, sizeof(header->speakerName))
    };
    return true;
}

void VoiceServer::handleJoin(ParsedPacket& packet){
    PROFILE_SCOPE("Handle Join");
    auto now = std::chrono::steady_clock::now();

    ClientInfo clientInfo{packet.senderAddr, now};
    clientManager.addClient(packet.senderKey, clientInfo);
    clientManager.joinRoom(packet.roomCode, packet.speakerName, clientInfo);

    LOG_INFO(tag(), "클라이언트 Join - 방: " << packet.roomCode << " / 화자: " << packet.speakerName);
}

void VoiceServer::handleVoiceData(ParsedPacket& packet){
    PROFILE_SCOPE("Handle Voice Data");
    auto now = std::chrono::steady_clock::now();

    // 클라이언트 갱신 (lastSeen 업데이트)
    ClientInfo clientInfo{packet.senderAddr, now};
    clientManager.addClient(packet.senderKey, clientInfo);
    clientManager.joinRoom(packet.roomCode, packet.speakerName, clientInfo);

    int cnt = 0;
    auto clientsSnapshot = clientManager.getClientsSnapshot();
    for(const auto& [otherKey, otherClient] : clientsSnapshot){
        if(otherKey != packet.senderKey){
            network.sendUdp(otherClient.addr, packet.rawBuffer, packet.rawSize);
            cnt++;
        }
    }

    if (packet.header->payloadSize > Voice::Config::LOG_PAYLOAD_MIN_SIZE) {
        LOG_INFO(tag(), "음성: " << packet.roomCode << "/" << packet.speakerName
                 << " " << packet.rawSize << "B → " << cnt << "명");
    }
}

void VoiceServer::handleLeave(ParsedPacket& packet){
    PROFILE_SCOPE("Handle Leave");
    if(!clientManager.hasClient(packet.senderKey)){
        LOG_WARN(tag(), "인증되지 않은 클라이언트의 Leave 패킷 수신 - 방: " << packet.roomCode << " / 화자: " << packet.speakerName);
        return;
    }
    clientManager.leaveRoom(packet.roomCode, packet.speakerName);
    clientManager.removeClient(packet.senderKey);

    LOG_INFO(tag(), "클라이언트 Leave - 방: " << packet.roomCode << " / 화자: " << packet.speakerName);
}
