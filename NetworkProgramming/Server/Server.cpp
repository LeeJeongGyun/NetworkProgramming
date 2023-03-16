
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

    // SOCKET OPTION
     
    // SO_KEEPALIVE -> 주기적으로 연결 상태 확인 여부 (TCP Only)
    // 상대방이 연결을 끊는다면?? -> TCP Layer에서 끊어진 연결 감진    
    bool enable = true;
    ::setsockopt(servSock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&enable), sizeof(enable));

    // SO_LINGER = 지연
    // onoff = 0이면 closesocket()이 바로 리턴, 아니면 linger초 만큼 대기 (default 0)
    // linger : 대기시간
    LINGER linger;
    linger.l_onoff = 1;
    linger.l_linger = 5;
    ::setsockopt(servSock, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&linger), sizeof(linger));

    // Half-Close
    // SD_SEND : send 금지
    // SD_RECV : recv 금지
    // SD_BOTH : 둘다 막음
    //::shutdown(servSock, SD_SEND);

    // SO_SNDBUF = 송신 버퍼 크기
    // SO_RCVBUF = 수신 버퍼 크기
    
    int sendBufSize;
    int optionLen = sizeof(sendBufSize);
    ::getsockopt(servSock, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&sendBufSize), &optionLen);

    std::cout << "Send Buf Size: " << sendBufSize << std::endl;

    int recvBufSize;
    optionLen = sizeof(recvBufSize);
    ::getsockopt(servSock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&recvBufSize), &optionLen);

    std::cout << "Recv Buf Size: " << recvBufSize << std::endl;

    // SO_REUSEADDR
    // IP 주소 및 PORT 재 사용
    {
        bool enable = true;
        ::setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&enable), sizeof(enable));
    }

    // IPPROTO_TCP
    // TCP_NODELAY = Nagle Algorithm 
    // 장점: 작은 패킷이 불필요하게 많이 생성되는 일 방지
    // 단점: 반응 시간 손해
    {
        bool enable = true;
        ::setsockopt(servSock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&enable), sizeof(enable));
    }


    ::closesocket(servSock);
    ::WSACleanup();
    return 0;
}
