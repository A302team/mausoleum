#include <string>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <vector>

#include "common/Platform.h"
#include "common/IClientManager.h"
#include "common/logging/Logger.h"
#include "common/ScopedTimer.h"
#include "voice/VoiceConstants.h"

using CLIENT_KEY = uint64_t;
struct ClientInfo {
    sockaddr_in addr;
    std::chrono::steady_clock::time_point lastSeen;
};

// IP:Port → uint64 key
CLIENT_KEY makeClientKey(const sockaddr_in& addr);

class VoiceClientManager : public IClientManager<CLIENT_KEY, ClientInfo> {
private:

    // 전체 클라이언트
    std::unordered_map<CLIENT_KEY, ClientInfo> clients;

    // roomCode -> members(clientKey)
    std::unordered_map<std::string, std::unordered_set<CLIENT_KEY>> rooms;

    // clientKey -> joined roomCode
    std::unordered_map<CLIENT_KEY, std::string> clientRooms;

    static constexpr int TIMEOUT_SECONDS = Voice::Config::CLIENT_TIMEOUT_SECONDS;
    mutable std::mutex mutex_;

public:

    void addClient(const CLIENT_KEY& key, const ClientInfo& data) override;
    void removeClient(const CLIENT_KEY& key) override;
    bool hasClient(const CLIENT_KEY& key) const override;
    size_t clientCount() const override;

    void joinRoom(const std::string& roomCode,
                  const CLIENT_KEY& key,
                  const ClientInfo& clientInfo)
    ;

    void leaveRoom(const CLIENT_KEY& key)
    ;

    std::vector<std::pair<CLIENT_KEY, ClientInfo>> getClientsSnapshot() const;
    std::vector<std::pair<CLIENT_KEY, ClientInfo>> getRoomClientsSnapshot(const std::string& roomCode) const;

    void cleanupStaleClients() override;
};
