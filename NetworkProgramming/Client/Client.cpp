
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

    SOCKADDR_IN servAdr;
    ::memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    //servAdr.sin_addr.s_addr = ::inet_addr("127.0.0.1");
    ::inet_pton(AF_INET, "127.0.0.1", &servAdr.sin_addr);
    servAdr.sin_port = htons(7777);

    if (SOCKET_ERROR == ::connect(clntSock, reinterpret_cast<SOCKADDR*>(&servAdr), sizeof(servAdr)))
    {
        ErrHandling();
        return 0;
    }

    std::cout << "Connected To Server!!" << std::endl;

    while (true)
    {
        char sendBuffer[100] = "Hello World";
        
        int sendSize = ::send(clntSock, sendBuffer, sizeof(sendBuffer), 0);
        if (sendSize == SOCKET_ERROR)
        {
            ErrHandling();
            return 0;
        }

        std::cout << "SendData!! Size: " << sizeof(sendBuffer) << std::endl;
        this_thread::sleep_for(1s);

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
    }

    ::closesocket(clntSock);
    
    // WinSock terminate
    ::WSACleanup();
    return 0;
}
