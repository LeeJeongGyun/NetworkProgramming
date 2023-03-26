
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

     // WSAEventSelect 모델
    vector<WSAEVENT> wsaEvents;
    vector<Session> sessions;
    sessions.reserve(100);

    WSAEVENT listenEvent = ::WSACreateEvent();
    wsaEvents.push_back(listenEvent);
    sessions.push_back(Session{ listenSock });

    if (SOCKET_ERROR == ::WSAEventSelect(listenSock, listenEvent, FD_ACCEPT | FD_CLOSE))
        return 0;

    while (true)
    {
        int index = ::WSAWaitForMultipleEvents(wsaEvents.size(), &wsaEvents[0], false, INFINITE, false);
        if (index == WSA_WAIT_FAILED)
            continue;
        
        index -= WSA_WAIT_EVENT_0;
        WSANETWORKEVENTS networkEvents;
        if (SOCKET_ERROR == ::WSAEnumNetworkEvents(sessions[index].socket, wsaEvents[index], &networkEvents))
            continue;

        // Listen Socket 체크
        if (networkEvents.lNetworkEvents & FD_ACCEPT)
        {
            if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
                continue;

            SOCKADDR_IN clntAdr;
            int clntAdrSz = sizeof(clntAdr);

            SOCKET clntSock = ::accept(sessions[index].socket, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntAdrSz);
            if (clntSock != INVALID_SOCKET)
            {
                std::cout << "Client Connect" << std::endl;

                WSAEVENT hEvent = ::WSACreateEvent();
                wsaEvents.push_back(hEvent);
                sessions.push_back(Session{ clntSock });

                if (SOCKET_ERROR == ::WSAEventSelect(clntSock, hEvent, FD_READ | FD_WRITE | FD_CLOSE))
                    return 0;
            }
        }

        if ((networkEvents.lNetworkEvents & FD_READ) || (networkEvents.lNetworkEvents & FD_WRITE))
        {
            if ( (networkEvents.lNetworkEvents & FD_READ) && (networkEvents.iErrorCode[FD_READ_BIT] != 0) ) continue;
            if ( (networkEvents.lNetworkEvents & FD_WRITE) && (networkEvents.iErrorCode[FD_WRITE_BIT] != 0) ) continue;

            Session& session = sessions[index];
            if (session.recvBytes == 0)
            {
                int recvSize = ::recv(session.socket, session.recvBuf, kBUF_SIZE, 0);
                if (recvSize <= 0)
                {
                    if (WSAGetLastError() == WSAEWOULDBLOCK) continue;
                    
                    // TODO 종료
                }

                session.recvBytes = recvSize;
                std::cout << "Recv Len: " << recvSize << std::endl;
            }
            
            if (session.recvBytes > session.sendBytes)
            {
                int sendSize = ::send(session.socket, &session.recvBuf[session.sendBytes], session.recvBytes - session.sendBytes, 0);

                session.sendBytes += sendSize;
                if (session.sendBytes == session.recvBytes)
                {
                    session.sendBytes = session.recvBytes = 0;
                }

                std::cout << "Send Data: " << sendSize << std::endl;
            }
        }

        if (networkEvents.lNetworkEvents & FD_CLOSE)
        {
            // TODO Remove
        }
    }

    ::closesocket(listenSock);
    ::WSACleanup();
    return 0;
}
