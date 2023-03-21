
#include <iostream>
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <thread>
using namespace std;
#pragma comment(lib, "ws2_32.lib")

int ErrHandling()
{
    int errCode = ::WSAGetLastError();
    std::cout << errCode << std::endl;
    return errCode;
}

int main()
{
    WSAData wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 0;

    SOCKET listenSock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET)
    {
        ErrHandling();
        return 0;
    }

    u_long lMode = 1; // Set non-blocking mode socket
    if (SOCKET_ERROR == ::ioctlsocket(listenSock, FIONBIO, &lMode))
    {
        ErrHandling();
        return 0;
    }

    SOCKADDR_IN servAdr;
    ::memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    servAdr.sin_port = ::htons(9999);

    if (SOCKET_ERROR == ::bind(listenSock, reinterpret_cast<SOCKADDR*>(&servAdr), sizeof(servAdr)))
    {
        ErrHandling();
        return 0;
    }

    if (SOCKET_ERROR == ::listen(listenSock, SOMAXCONN))
    {
        ErrHandling();
        return 0;
    }

    SOCKADDR_IN clntAdr;
    int clntAdrSz = sizeof(clntAdr);

    // Accept
    while (true)
    {
        SOCKET clntSock = ::accept(listenSock, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntAdrSz);
        if (clntSock == INVALID_SOCKET)
        {
            if (::WSAGetLastError() == WSAEWOULDBLOCK)
                continue;
            else
                break;
        }

        std::cout << "Client Connected" << std::endl;

        // Recv
        while (true)
        {
            char recvBuf[1000];
            int recvSize = ::recv(clntSock, recvBuf, sizeof(recvBuf), 0);
            if (recvSize == SOCKET_ERROR)
            {
                if (::WSAGetLastError() == WSAEWOULDBLOCK)
                    continue;
                
                ErrHandling();
                return 0;
            }
            else if (recvSize == 0)
            {
                std::cout << "Client Terminate" << std::endl;
                break;
            } 
            
            std::cout << "Recv Len: " << recvSize << std::endl;
            std::cout << "Recv Data: " << recvBuf << std::endl;
            
            // Send
            while (true)
            {
                int sendSize = ::send(clntSock, recvBuf, recvSize, 0);
                if (sendSize == SOCKET_ERROR)
                {
                    if (::WSAGetLastError() == WSAEWOULDBLOCK) continue;
                }
                else if (sendSize == 0)
                {
                    std::cout << "Client Connection Terminate" << std::endl;
                }
                
                break;
            }
        }
    }


    ::closesocket(listenSock);
    ::WSACleanup();
    return 0;
}
