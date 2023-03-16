
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

    SOCKET servSock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (servSock == INVALID_SOCKET)
    {
        ErrHandling();
        return 0;
    }

    SOCKADDR_IN servAdr;
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    servAdr.sin_port = ::htons(7777);

    if (SOCKET_ERROR == bind(servSock, reinterpret_cast<SOCKADDR*>(&servAdr), sizeof(servAdr)))
    {
        ErrHandling();
        return 0;
    }

    while (true)
    {
        char recvBuf[1000];
        SOCKADDR_IN clntAdrInfo;
        memset(&clntAdrInfo, 0, sizeof(clntAdrInfo));
        int clntAdrSize = sizeof(clntAdrInfo);
        int recvSize = ::recvfrom(servSock, recvBuf, sizeof(recvBuf), 0, reinterpret_cast<SOCKADDR*>(&clntAdrInfo), &clntAdrSize);
    
        if (recvSize == SOCKET_ERROR)
        {
            ErrHandling();
            return 0;
        }
        else if (recvSize == 0)
        {
            std::cout << "Client Exit!!" << std::endl;
            break;
        }

        std::cout << "Recv Data Len: " << recvSize << std::endl;
        std::cout << "Recv Data: " << recvBuf << std::endl;

        int sendSize = ::sendto(servSock, recvBuf, sizeof(recvBuf), 0, reinterpret_cast<SOCKADDR*>(&clntAdrInfo), sizeof(clntAdrInfo));
        if (sendSize <= 0)
        {
            ErrHandling();
            return 0;
        }

        std::cout << "Send Data Len: " << sendSize << std::endl;
        this_thread::sleep_for(1s);
    }

    ::closesocket(servSock);
    ::WSACleanup();
    return 0;
}
