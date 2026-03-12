#include <string>
#include <unordered_map>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <vector>

#include "common/Platform.h"
#include "common/IClientManager.h"
#include "common/logging/Logger.h"
#include "common/ScopedTimer.h"

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

    // roomCode → speaker → clientKey
    std::unordered_map<std::string, std::unordered_map<std::string, CLIENT_KEY>> rooms;

    const int TIMEOUT_SECONDS = 300;
    mutable std::mutex mutex_;

public:

    void addClient(const CLIENT_KEY& key, const ClientInfo& data) override;
    void removeClient(const CLIENT_KEY& key) override;
    bool hasClient(const CLIENT_KEY& key) const override;
    size_t clientCount() const override;

    void joinRoom(const std::string& roomCode,
                  const std::string& speaker,
                  const ClientInfo& clientInfo)
    ;

    void leaveRoom(const std::string& roomCode,
                   const std::string& speaker)
    ;

    std::vector<std::pair<CLIENT_KEY, ClientInfo>> getClientsSnapshot() const;

    void cleanupStaleClients() override;
};
