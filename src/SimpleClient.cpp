
#include <iostream>
#include <algorithm>
#include <sstream>
#include <utility>

#include <cerrno>
#include <cassert>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/signal.h>

#include "SimpleClient.h"

SimpleClient::SimpleClient() : m_hSocket(-1), m_Port(-1), m_HostIP(0)
{

}

int SimpleClient::Init(const char *pName, const char *pIP, int port)
{
    int ret;
    std::string name(pName);
    m_selfAttr.UesrName = name;

    ret = inet_pton(AF_INET, pIP, &m_HostIP);
    if(ret != 1)
    {
        std::cout << "Invaild IP address!Please enter IPV4 addr!" << std::endl;
        goto ERR;
    }
    m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_hSocket < 0)
        goto ERR;

    return 0;
    ERR:
    std::cerr << "Fail in client init" << std::endl;

}

int SimpleClient::Start()
{
    int ret;
    ret = ConnectHost();
    if(ret < 0)
        return ret;
    ret = Login();
    //客户端目前只能收
    if(ret == HANDLEMSGRESULT_LOGINAUTHSUCCESS)
    {
        while(1)
        {

        }
    }
    //remove..
    return 0;
}

int SimpleClient::ConnectHost()
{
    int ret;
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(m_Port);
    servaddr.sin_addr.s_addr = htonl(m_HostIP);

    assert(m_hSocket >= 0);

    ret = connect(m_hSocket, (sockaddr *)&servaddr, sizeof(servaddr));
    if(ret < 0)
    {
        std::cerr << errno << "  " << strerror(errno) << std::endl;
        return ret;
    }
    ret = AddSession(m_hSocket);
    if(ret < 0)
        return -1;
    assert(m_Guests.size() == 2);
    m_hostSession = m_Guests[1];
    assert(m_hostSession->fd == m_hSocket);

    return ret;
}

int SimpleClient::Login()
{
    char *pBuf = new char[1024];
    if(!pBuf)
        return -1;
    assert(m_SelfSession->pSession);
    int nameLen = strlen(m_SelfSession->pSession->GetName());
    assert(nameLen > 0);
    LoginMessage *pLogin = new(pBuf) LoginMessage(nameLen);
    memcpy(pLogin->Payload, m_SelfSession->pSession->GetName(), nameLen);
    return ret;
}

SimpleMsgHdr *SimpleClient::ClinetRecv()
{
    int ret = RecevieMessage(m_hSocket, m_RecvBuf, 2048);
    if(ret < 0)
        ;
    if(ret == 0)
        ;
    SimpleMsgHdr *pMsg = reinterpret_cast<SimpleMsgHdr*>(m_RecvBuf);
    assert(pMsg->FrameHead == MSG_FRAME_HEADER);
    return pMsg;
}
