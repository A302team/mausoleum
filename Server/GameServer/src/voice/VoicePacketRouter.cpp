#include "voice/VoicePacketRouter.h"
#include <utility>

void VoicePacketRouter::registerHandler(VoicePacketType packetType, Handler handler) {
    handlers[packetType] = std::move(handler);
}

void VoicePacketRouter::dispatch(ParsedPacket& packet) {
    auto it = handlers.find(packet.header->packetType);
    if (it != handlers.end()) {
        LOG_INFO("VoicePacketRouter", "Dispatching packet type: " << static_cast<int>(packet.header->packetType));
        it->second(packet); // 해당 패킷 타입에 등록된 핸들러 호출
    } else {
        LOG_WARN("VoicePacketRouter",
            "No handler registered for packet type: " << static_cast<int>(packet.header->packetType));
    }
}
