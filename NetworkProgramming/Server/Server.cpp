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
};

enum IO_TYPE
{
    READ,
    WRITE,
    CONNECT,
    ACCEPT
};

struct OverlappedEx
{
    WSAOVERLAPPED overlapped = {};
    int ioType;
};

void WorkerThread(HANDLE iocpHandle)
{
    while (true)
    {
        DWORD bytesTransferred = 0;
        Session* pSession = nullptr;
        OverlappedEx* pOverlappedEx = nullptr;
        BOOL ret = ::GetQueuedCompletionStatus(iocpHandle, &bytesTransferred, (PULONG_PTR)&pSession, 
            (LPOVERLAPPED*)&pOverlappedEx, INFINITE);

        if (ret == 0 || bytesTransferred == 0) continue;

        std::cout << "Recv Data IOCP" << std::endl;

        WSABUF wsaBuf;
        wsaBuf.buf = pSession->recvBuf;
        wsaBuf.len = kBUF_SIZE;

        DWORD recvLen = 0;
        DWORD falgs = 0;
        ::WSARecv(pSession->socket, &wsaBuf, 1, &recvLen, &falgs, &pOverlappedEx->overlapped, NULL);
    }
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

    std::vector<Session*> sessionManager;
    HANDLE iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    std::vector<thread> v;
    for (int i = 0; i < 5; ++i)
    {
        v.push_back(thread([=]() { WorkerThread(iocpHandle); }));
    }

    // Overlapped 모델 ( 이벤트 기반 )
    while (true)
    {
        SOCKET clntSock;
        SOCKADDR_IN clntAdr;
        int clntAdrSz = sizeof(clntAdr);
        clntSock = ::accept(listenSock, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntAdrSz);
        if (clntSock == INVALID_SOCKET) return 0;

        Session* pSession = new Session();
        pSession->socket = clntSock;
        sessionManager.push_back(pSession);

        std::cout << "Client Connect" << std::endl;
        
        // 등록
        ::CreateIoCompletionPort((HANDLE)clntSock, iocpHandle, /*Key*/(ULONG_PTR)pSession, 0);

        WSABUF wsaBuf;
        wsaBuf.buf = pSession->recvBuf;
        wsaBuf.len = kBUF_SIZE;

        OverlappedEx* pOverlappedEx = new OverlappedEx;
        pOverlappedEx->ioType = IO_TYPE::READ;
        DWORD recvLen = 0;
        DWORD falgs = 0;

        ::WSARecv(clntSock, &wsaBuf, 1, &recvLen, &falgs, &pOverlappedEx->overlapped, NULL);

    }

    for (int i = 0; i < 5; ++i) v[i].join();

    ::closesocket(listenSock);
    ::WSACleanup();
    return 0;
}