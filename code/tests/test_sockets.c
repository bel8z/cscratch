#include "foundation/error.h"
#include "foundation/threading.h"
#include "foundation/time.h"

#include "foundation/win32.inl"

// #include <iphlpapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <stdio.h>

#define DEFAULT_PORT "27015"

typedef struct Platform Platform;
typedef struct CommandLine CommandLine;

typedef struct Result
{
    I32 code;
} Result;

typedef HANDLE Iocp;

CF_INTERNAL Iocp
iocpCreate(Usize max_concurrency)
{
    CF_ASSERT(max_concurrency <= U32_MAX, "Overflow");
    return CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, (DWORD)max_concurrency);
}

CF_INTERNAL bool
iocpBindSocket(Iocp iocp, SOCKET socket)
{
    return (iocp == CreateIoCompletionPort((HANDLE)socket, iocp, 0, 0));
}

CF_INTERNAL void
serverFn(void *data)
{
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
    Iocp iocp = iocpCreate(1);

    //===================================//
    fprintf(stderr, "Server: Initializing winsock\n");

    if (getaddrinfo(NULL, DEFAULT_PORT, &hint, &info)) goto CLEANUP;

    CF_ASSERT(info->ai_addrlen == sizeof(struct sockaddr_in), "Logic error");
    struct sockaddr_in *address = (struct sockaddr_in *)info->ai_addr;

    char addrbuf[256] = {0};
    fprintf(stderr, "Server: Server address: %s\n",
            inet_ntop(address->sin_family, &address->sin_addr, addrbuf, CF_ARRAY_SIZE(addrbuf)));

    //===================================//
    fprintf(stderr, "Server: Creating server socket\n");

    listener = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (listener == INVALID_SOCKET) goto CLEANUP;

    iocpBindSocket(iocp, listener);

    //===================================//
    fprintf(stderr, "Server: Binding server socket\n");

    if (bind(listener, info->ai_addr, (I32)info->ai_addrlen)) goto CLEANUP;

    //===================================//
    fprintf(stderr, "Server: Start listening\n");

    // The backlog parameter is set to SOMAXCONN: this value is a special constant that instructs
    // the Winsock provider for this socket to allow a maximum reasonable number of pending
    // connections in the queue.
    if (listen(listener, SOMAXCONN) == SOCKET_ERROR) goto CLEANUP;

    //===================================//
    fprintf(stderr, "Server: Accept client connection\n");

    client = accept(listener, NULL, NULL);
    if (client == INVALID_SOCKET) goto CLEANUP;

    //===================================//
    fprintf(stderr, "Server: Handling client connection\n");

    // Receive until the peer shuts down the connection
    I32 received = 0;
    I32 sent = 0;
    char recvbuf[512];
    do
    {
        received = recv(client, recvbuf, CF_ARRAY_SIZE(recvbuf), 0);
        if (received > 0)
        {
            fprintf(stderr, "Server: Bytes received: %d\n", received);

            // Echo the buffer back to the sender
            sent = send(client, recvbuf, received, 0);
            if (sent == SOCKET_ERROR) goto CLEANUP;

            fprintf(stderr, "Server: Bytes sent: %d\n", sent);
        }
        else if (received == 0)
        {
            fprintf(stderr, "Server: Connection closing...\n");
        }
        else
        {
            goto CLEANUP;
        }

    } while (received > 0);

    //===================================//
    fprintf(stderr, "Server: Shutting down sending side of the connection\n");
    shutdown(client, SD_SEND);

CLEANUP:
    if (client != INVALID_SOCKET) closesocket(client);
    if (listener != INVALID_SOCKET) closesocket(listener);
    if (info) freeaddrinfo(info);

    Result *result = data;
    result->code = WSAGetLastError();
}

CF_INTERNAL void
clientFn(void *data)
{
    cfSleep(timeDurationMs(2000));

    ADDRINFO *info = NULL;
    ADDRINFO hint = {
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
        .ai_family = AF_UNSPEC,
    };

    SOCKET connection = INVALID_SOCKET;

    //===================================//
    fprintf(stderr, "Client: Initializing client\n");

    if (getaddrinfo("127.0.0.1", DEFAULT_PORT, &hint, &info)) goto CLEANUP;

    //===================================//
    fprintf(stderr, "Client: Attempting to connect to server\n");

    for (ADDRINFO *ptr = info; ptr; ptr = ptr->ai_next)
    {
        connection = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connection == INVALID_SOCKET) break;

        if (!connect(connection, ptr->ai_addr, (I32)ptr->ai_addrlen)) break;

        closesocket(connection);
        connection = INVALID_SOCKET;
    }

    if (connection == INVALID_SOCKET)
    {
        fprintf(stderr, "Client: Unable to connect to server!\n");
        goto CLEANUP;
    }

    //===================================//
    fprintf(stderr, "Client: Sending an initial buffer\n");

    char sendbuf[] = "Beccati questo!";
    I32 sent = send(connection, sendbuf, CF_ARRAY_SIZE(sendbuf) - 1, 0);
    if (sent == SOCKET_ERROR) goto CLEANUP;

    fprintf(stderr, "Client: Bytes Sent: %d\n", sent);

    //===================================//
    fprintf(stderr, "Client: Shutting down sending side of the connection\n");

    if (shutdown(connection, SD_SEND)) goto CLEANUP;

    //===================================//
    fprintf(stderr, "Client: Receive until the peer closes the connection\n");

    char recvbuf[512] = {0};
    I32 received = 0;
    do
    {
        received = recv(connection, recvbuf, CF_ARRAY_SIZE(recvbuf), 0);
        if (received > 0)
        {
            fprintf(stderr, "Client: Bytes received: %d\n", received);
        }
        else if (received == 0)
        {
            fprintf(stderr, "Client: Connection closed\n");
        }
        else
        {
            goto CLEANUP;
        }
    } while (received > 0);

CLEANUP:
    if (connection != INVALID_SOCKET) closesocket(connection);
    if (info) freeaddrinfo(info);

    Result *result = data;
    result->code = WSAGetLastError();
}

I32
consoleMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(platform);
    CF_UNUSED(cmd_line);

    fprintf(stderr, "Initializing winsock\n");

    WSADATA wsa_data;
    I32 code = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (code) return code;

    Result results[2] = {{0}, {0}};
    Cstr names[2] = {
        "Client",
        "Server",
    };
    CfThread threads[2] = {
        cfThreadStart(clientFn, .args = results),
        cfThreadStart(serverFn, .args = results + 1),
    };

#if 0
    Usize mask = 0;
    while (mask != 3)
    {
        Usize done = mask;
        Usize signaled = cfThreadWaitAny(threads, CF_ARRAY_SIZE(threads), DURATION_INFINITE);

        switch (signaled)
        {
            case USIZE_MAX:
                // ERROR
                mask = 3;
                break;
            case CF_ARRAY_SIZE(threads):
                // NOP
                break;
            default:
                Usize flag = (1 << signaled);
                if ((done |= flag) != mask)
                {
                    fprintf(stderr, "%s exited with code: %d\n", names[signaled],
                            results[signaled].code);
                }
                break;
        }

        mask = done;
    }
#else
    cfThreadWaitAll(threads, CF_ARRAY_SIZE(threads), DURATION_INFINITE);
    fprintf(stderr, "%s exited with code: %d\n", names[0], results[0].code);
    fprintf(stderr, "%s exited with code: %d\n", names[1], results[1].code);
#endif

    WSACleanup();

    if (results[0].code) return results[0].code;
    if (results[1].code) return results[1].code;

    return 0;
}
