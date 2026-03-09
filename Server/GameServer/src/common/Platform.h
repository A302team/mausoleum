#pragma once

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <mswsock.h>

    using SocketType = SOCKET;
    using SockLenType = int;

    #define INVALID_SOCK INVALID_SOCKET
    #define SOCK_ERROR SOCKET_ERROR
    #define CLOSE_SOCKET closesocket
    #define GET_LAST_ERROR() WSAGetLastError()
    
    #define ERR_CONNRESET WSAECONNRESET
    #define ERR_WOULDBLOCK WSAEWOULDBLOCK

    #define FUNC_SIG __FUNCSIG__
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <cstring>

    using SocketType = int;
    using SockLenType = socklen_t;

    #define INVALID_SOCK (-1)
    #define SOCK_ERROR (-1)
    #define CLOSE_SOCKET close
    #define GET_LAST_ERROR() errno
    
    #define ERR_CONNRESET ECONNRESET
    #define ERR_WOULDBLOCK EWOULDBLOCK

    #define FUNC_SIG __PRETTY_FUNCTION__
#endif
