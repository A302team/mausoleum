#pragma once
#include <cstddef>

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