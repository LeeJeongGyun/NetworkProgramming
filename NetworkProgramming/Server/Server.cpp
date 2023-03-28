#include <iostream>
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#include <thread>
#include <vector>
using namespace std;
#pragma comment(lib, "ws2_32.lib")

int ErrHandling()
{
    int errCode = ::WSAGetLastError();
    std::cout << errCode << std::endl;
    return errCode;
}

const int kBUF_SIZE = 1000;
struct Session
{
    WSAOVERLAPPED overlapped = {};
    SOCKET socket = INVALID_SOCKET;
    char recvBuf[kBUF_SIZE];
    int recvBytes = 0;
};

void WINAPI CallBackRcv(DWORD error, DWORD recvLen, LPWSAOVERLAPPED pOverlapped, DWORD flags)
{
    cout << "CALL BACK Data Recv Len = " << recvLen << std::endl;
    // TODO 에코라면 WSASend();

    // 이런식으로 사용
    Session* pSession = (Session*)pOverlapped;
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

    std::cout << "Accept" << std::endl;

    // Overlapped 모델 ( 이벤트 기반 )
    while (true)
    {
        SOCKET clntSock;
        SOCKADDR_IN clntAdr;
        int clntAdrSz = sizeof(clntAdr);
        while (true)
        {
            clntSock = ::accept(listenSock, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntAdrSz);
            if (clntSock != INVALID_SOCKET) break;

            if (::WSAGetLastError() == WSAEWOULDBLOCK) continue;

            return -1;
        }

        std::cout << "Client Connect" << std::endl;

        Session session{ clntSock, };
        
        while (true)
        {
            WSABUF wsaBuf;
            wsaBuf.buf = session.recvBuf;
            wsaBuf.len = kBUF_SIZE;

            DWORD recvLen = 0;
            DWORD flags = 0;

            if (SOCKET_ERROR == ::WSARecv(clntSock, &wsaBuf, 1, &recvLen, &flags, &session.overlapped, CallBackRcv))
            {
                if (::WSAGetLastError() == WSA_IO_PENDING)
                {
                    ::SleepEx(WSA_INFINITE, TRUE);
                }
                else
                {
                    // TODO : 문제 있는 상황
                    break;
                }
            }
            else
            {
                std::cout << "Data Recv Len = " << recvLen << std::endl;
            }

        }
        ::closesocket(session.socket);
    }

    ::closesocket(listenSock);
    ::WSACleanup();
    return 0;
}