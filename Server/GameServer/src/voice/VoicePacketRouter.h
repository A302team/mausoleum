#include <functional>
#include <unordered_map>
#include "common/Platform.h"
#include "common/Logger.h"
#include "VoicePacketType.h"

#pragma pack(push, 1)
struct VoicePacketHeader {
    VoicePacketType packetType; // 0: Join, 1: Voice Data, 2: Leave
    char roomCode[16];        // 룸 코드 (가변 가능하지만 고정 크기 16바이트로 제한)
    char speakerName[32];     // 발화자 이름 고정 (최대 32바이트)
    uint32_t payloadSize;     // 뒤에 따라오는 음성 데이터 
};

#pragma pack(pop)

struct ParsedPacket {
    VoicePacketHeader* header;
    char* rawBuffer;
    int rawSize;
    sockaddr_in senderAddr;
    uint64_t senderKey;
    std::string roomCode;
    std::string speakerName;
};

class VoicePacketRouter {
public:
    using Handler = std::function<void(ParsedPacket&)>;

    void registerHandler(VoicePacketType packetType, Handler handler) {
        handlers[packetType] = handler;
    }

    void dispatch(ParsedPacket& packet) {
        auto it = handlers.find(packet.header->packetType);
        if(it != handlers.end()) {
            it->second(packet);
        } else {
            LOG_WARN("VoicePacketRouter", "No handler registered for packet type: " << static_cast<int>(packet.header->packetType));
        }
    }
private:
    std::unordered_map<VoicePacketType, Handler> handlers;
};