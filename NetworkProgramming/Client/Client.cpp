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
    char sendBuf[100] = "Hello World";
    HANDLE hEvent = ::WSACreateEvent();
    WSAOVERLAPPED overlapped = {};
    overlapped.hEvent = hEvent;
    while (true)
    {
        WSABUF wsaBuf;
        wsaBuf.buf = sendBuf;
        wsaBuf.len = sizeof(sendBuf);

        DWORD sendLen = 0;
        DWORD flags = 0;

        if (SOCKET_ERROR == ::WSASend(clntSock, &wsaBuf, 1, &sendLen, flags, &overlapped, nullptr))
        {
            if (::WSAGetLastError() == WSA_IO_PENDING)
            {
                ::WSAWaitForMultipleEvents(1, &hEvent, true, WSA_INFINITE, false);
                ::WSAGetOverlappedResult(clntSock, &overlapped, &sendLen, false, &flags);
            }
            else
            {
                // TODO 종료
                break;
            }
        }

        std::cout << "Send Data Len = " << sendLen << std::endl;
        this_thread::sleep_for(100ms);
    }

    ::closesocket(clntSock);
    ::WSACloseEvent(hEvent);

    // WinSock terminate
    ::WSACleanup();
    return 0;
}