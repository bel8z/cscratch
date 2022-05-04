#include "foundation/win32.inl"

// #include <iphlpapi.h>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"

typedef struct Platform Platform;
typedef struct CommandLine CommandLine;

#define CHECK(result)               \
    if (result)                     \
    {                               \
        result = WSAGetLastError(); \
        goto CLEANUP;               \
    }

I32
platformMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(platform);
    CF_UNUSED(cmd_line);

    I32 result = 0;

    ADDRINFO *info = NULL;
    ADDRINFO hint = {
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
        // IPv4 address family
        .ai_family = AF_INET,
        // the returned socket address will be used in a call to the bind function
        .ai_flags = AI_PASSIVE,
    };

    SOCKET listener = INVALID_SOCKET;
    SOCKET client = INVALID_SOCKET;

    // Initialize Winsock
    WSADATA wsaData;
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result) goto CLEANUP;

    if (getaddrinfo(NULL, DEFAULT_PORT, &hint, &info))
    {
        result = WSAGetLastError();
        goto CLEANUP;
    }

    listener = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (listener == INVALID_SOCKET)
    {
        result = WSAGetLastError();
        goto CLEANUP;
    }

    if (bind(listener, info->ai_addr, (I32)info->ai_addrlen))
    {
        result = WSAGetLastError();
        goto CLEANUP;
    }

    // The backlog parameter is set to SOMAXCONN: this value is a special constant that instructs
    // the Winsock provider for this socket to allow a maximum reasonable number of pending
    // connections in the queue.
    if (listen(listener, SOMAXCONN) == SOCKET_ERROR)
    {
        result = WSAGetLastError();
        goto CLEANUP;
    }

    // Accept a client socket
    client = accept(listener, NULL, NULL);
    if (client == INVALID_SOCKET)
    {
        result = WSAGetLastError();
        goto CLEANUP;
    }

CLEANUP:
    if (client != INVALID_SOCKET) closesocket(client);
    if (listener != INVALID_SOCKET) closesocket(listener);
    if (info) freeaddrinfo(info);
    WSACleanup();

    return result;
}
