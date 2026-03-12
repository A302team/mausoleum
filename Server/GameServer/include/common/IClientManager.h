#pragma once
#include <cstddef>

/**
 * @brief 클라이언트 관리자 인터페이스
 * 
 * @tparam TKey 클라이언트 식별 키 타입 (예: IP:Port → uint64)
 * @tparam TData 클라이언트 정보 데이터 타입 (예: sockaddr_in, lastSeen 등)
 */
template<typename TKey, typename TData>
class IClientManager {
public:
    virtual ~IClientManager() = default;

    virtual void addClient(const TKey& key, const TData& data) = 0;
    virtual void removeClient(const TKey& key) = 0;
    virtual bool hasClient(const TKey& key) const = 0;
    virtual size_t clientCount() const = 0;
    virtual void cleanupStaleClients() = 0;
};