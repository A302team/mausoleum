#include "VoiceServer.h"


bool VoiceServer::initSocket(int port) {
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock == INVALID_SOCK){
        LOG_ERROR(tag(), "소켓 생성 실패: " << GET_LAST_ERROR());
        return false;
    }

#ifdef _WIN32
    BOOL bFalse = FALSE;
    DWORD dwBtyes = 0;
    WSAIoctl(sock, SIO_UDP_CONNRESET, &bFalse, sizeof(bFalse), 
             NULL, 0, &dwBtyes, NULL, NULL);
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if(bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCK_ERROR){
        LOG_ERROR(tag(), "소켓 바인딩 실패: " << GET_LAST_ERROR());
        CLOSE_SOCKET(sock);
        return false;
    }
    return true;
}
void VoiceServer::runLoop() {
    char buffer[65536];
    sockaddr_in clinetAddr;
    SockLenType addrLen = sizeof(clinetAddr);
    while(running){
        int bytesRecv = recvfrom(sock, buffer, sizeof(buffer), 
                                0, (sockaddr*)&clinetAddr, &addrLen);
        if(bytesRecv == SOCK_ERROR){
            int err = GET_LAST_ERROR();
            if(err == ERR_CONNRESET || err == ERR_WOULDBLOCK) continue;
            LOG_ERROR(tag(), "데이터 수신 실패: " << err);
            continue;
        }
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(
                now - lastCleanupTime).count() > CLEANUP_INTERVAL_SECONDS) {
            clientManager.cleanupStaleClients();
            lastCleanupTime = now;
        }
        auto clientKey = makeClientKey(clinetAddr);
        ParsedPacket packet{
            (VoicePacketHeader*)buffer,
            buffer,
            bytesRecv,
            clinetAddr,
            clientKey,
            std::string(((VoicePacketHeader*)buffer)->roomCode),
            std::string(((VoicePacketHeader*)buffer)->speakerName)
        };
        packetRouter.dispatch(packet);
    }
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
    for(const auto& [otherKey, otherClient] : clientManager.getClients()){
        if(otherKey != packet.senderKey){
            int r = sendto(sock, packet.rawBuffer, packet.rawSize, 0, (sockaddr*)&otherClient.addr, sizeof(otherClient.addr));
            if(r != SOCK_ERROR) cnt++;
        }
    }

    if (packet.header->payloadSize > 2) {
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