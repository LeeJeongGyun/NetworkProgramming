
#include <iostream>
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <thread>
using namespace std;
#pragma comment(lib, "ws2_32.lib")

void ErrHandling()
{
    std::cout << WSAGetLastError() << std::endl;
}

int main()
{
    WSAData wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 0;

    SOCKET lisetnSock;
    lisetnSock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (lisetnSock == INVALID_SOCKET)
    {
        ErrHandling();
        return 0;
    }

    SOCKADDR_IN servAdr;
    ::memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = htonl(INADDR_ANY); // 알아서 골라.
    servAdr.sin_port = htons(7777);

    if (SOCKET_ERROR == ::bind(lisetnSock, reinterpret_cast<SOCKADDR*>(&servAdr), sizeof(servAdr)))
    {
        ErrHandling();
        return 0;
    }

    if (::listen(lisetnSock, SOMAXCONN) == SOCKET_ERROR)
    {
        ErrHandling();
        return 0;
    }

    SOCKADDR_IN clntAdr;
    int clntSize = sizeof(clntAdr);

    SOCKET clntSock = ::accept(lisetnSock, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntSize);
    if (clntSock == INVALID_SOCKET)
    {
        ErrHandling();
        return 0;
    }

    // client connection success
    char clntIP[16];
    ::inet_ntop(AF_INET, &clntAdr.sin_addr, clntIP, sizeof(clntIP));

    std::cout << "Client Ip: " << clntIP << std::endl;


    while (true)
    {
        char recvBuffer[1000];
        int recvSize = ::recv(clntSock, recvBuffer, sizeof(recvBuffer), 0);
        
        if (recvSize == SOCKET_ERROR)
        {
            ErrHandling();
            return -1;
        }
        else if (recvSize == 0)
        {
            std::cout << "Client Exit" << std::endl;
            break;
        }
        
        std::cout << "Recv Data: " << recvBuffer << std::endl;
        std::cout << "Recv Data Len: " << recvSize << std::endl;

        int sendSize = ::send(clntSock, recvBuffer, recvSize, 0);
        if (sendSize == SOCKET_ERROR)
        {
            ErrHandling();
            return 0;
        }

        std::cout << "SendData!! Size: " << sendSize << std::endl;
        this_thread::sleep_for(1s);
    }



    ::WSACleanup();
    return 0;
}
