
#include <iostream>
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <thread>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

int G_ErrCode;

void ErrHandling()
{
    std::cout << WSAGetLastError() << std::endl;
}

int main()
{
    this_thread::sleep_for(1s);

    WSAData wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 0;

    SOCKET clntSock;
    clntSock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clntSock == INVALID_SOCKET)
    {
        ErrHandling();
        return 0;
    }

    u_long lMode = 1;
    ::ioctlsocket(clntSock, FIONBIO, &lMode);

    SOCKADDR_IN servAdr;
    ::memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    ::InetPtonA(AF_INET, "127.0.0.1", &servAdr.sin_addr);
    servAdr.sin_port = htons(9999);

    while (true)
    {
        if (SOCKET_ERROR == ::connect(clntSock, reinterpret_cast<SOCKADDR*>(&servAdr), sizeof(servAdr)))
        {
            if (::WSAGetLastError() == WSAEWOULDBLOCK) continue;

            if (::WSAGetLastError() == WSAEISCONN || ::WSAGetLastError() == WSAEALREADY) break;

            ErrHandling();
            return 0;
        }
    }

    std::cout << "Connection Success" << std::endl;

    // Send
    while (true)
    {
        char sndBuf[100] = "Hello World";
        int sendSize = ::send(clntSock, sndBuf, sizeof(sndBuf), 0);
        if (sendSize == SOCKET_ERROR)
        {
            if (::WSAGetLastError() == WSAEWOULDBLOCK) continue;
        }
        else if (sendSize == 0)
        {
            std::cout << "Client Connection Terminate" << std::endl;
        }

        std::cout << "Send Len: " << sendSize << std::endl;
           
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
            break;
        }
    }
   
    ::closesocket(clntSock);
    
    // WinSock terminate
    ::WSACleanup();
    return 0;
}
