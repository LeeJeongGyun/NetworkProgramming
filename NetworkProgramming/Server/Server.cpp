
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
    SOCKET socket = INVALID_SOCKET;
    char recvBuf[kBUF_SIZE];
    int recvBytes = 0;
    int sendBytes = 0;
};

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

    // select 모델
    
    std::vector<Session> sessions;
    sessions.reserve(1000);
    FD_SET readSet;
    FD_SET writeSet;

    while (true)
    {
        // 소켓 set 초기화
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);

        // ListenSocket 등록
        FD_SET(listenSock, &readSet);

        // 소켓 등록
        for (Session& s : sessions)
        {
            if (s.recvBytes <= s.sendBytes)
                FD_SET(s.socket, &readSet);
            else
                FD_SET(s.socket, &writeSet);
        }

        FD_SET copyRead, copyWrite;
        copyRead = readSet;
        copyWrite = writeSet;

        // [옵션] 마지막 timeout 인자 설정 가능
        int retVal = ::select(0, &readSet, &writeSet, nullptr, nullptr);
        if (retVal == SOCKET_ERROR)
        {
            ErrHandling();
            break;
        }

        // Listener 소켓 체크
        if (FD_ISSET(listenSock, &readSet))
        {
            SOCKADDR_IN clntAdr;
            int clntAdrSz = sizeof(clntAdr);

            SOCKET clntSock = ::accept(listenSock, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntAdrSz);
            if (clntSock != INVALID_SOCKET)
            {
                std::cout << "Client Connected" << std::endl;
                sessions.push_back(Session{ clntSock });
            }
        }

        for (Session& s : sessions)
        {
            // Read
            if (FD_ISSET(s.socket, &readSet))
            {
                int recvSize = ::recv(s.socket, s.recvBuf, kBUF_SIZE, 0);
                if (recvSize <= 0)
                {
                    // TODO : sessions 제거
                    continue;
                }

                s.recvBytes = recvSize;
            }

            // Write
            if (FD_ISSET(s.socket, &writeSet))
            {
                // 블로킹 소켓 -> 모든 데이터 다 보냄
                // 논 블로킹 소켓 -> 일부 데이터만 보낼 수 있음.(상대의 수신 버퍼 상태에 의존)
                int sendSize = ::send(s.socket, &s.recvBuf[s.sendBytes], s.recvBytes - s.sendBytes, 0);
                if (sendSize == SOCKET_ERROR)
                {
                    // TODO : sessions 제거
                    continue;
                }

                s.sendBytes += sendSize;
                if (s.sendBytes == s.recvBytes)
                {
                    s.sendBytes = s.recvBytes = 0;
                }
            }
        }
    }




    ::closesocket(listenSock);
    ::WSACleanup();
    return 0;
}
