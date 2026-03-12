#pragma once
#include "common/Platform.h"

class SocketPlatform {
public:
    virtual ~SocketPlatform() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool setNonBlocking(SocketType sock) = 0;
    virtual void configureUdpSocket(SocketType sock) = 0;
    virtual int lastError() const = 0;
};

SocketPlatform& GetSocketPlatform();
