
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

SimpleClient::SimpleClient() : m_hSocket(-1), m_Port(-1), m_HostIP(0), m_bWork(false)
{

}

int SimpleClient::Init(const char *pName, const char *pIP, int port)
{
    assert(m_hSocket == -1);
    assert(pName);
    assert(pIP);
    assert(this);
    int ret;
    std::string name(pName);
    m_Port = port;
    m_SelfAttr.UesrName = name;

    ret = inet_pton(AF_INET, pIP, &m_HostIP);
    if(ret != 1)
    {
        std::cout << "Invaild IP address!Please enter IPV4 addr!" << std::endl;
        goto ERR;
    }
    m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_hSocket < 0)
        goto ERR;

    ret = m_Listener.InitListener();
    if(ret < 0)
    {
        ret = -2;
        goto ERR;
    }

    return 0;

    ERR:
    if(m_hSocket >=0)
    {
        close(m_hSocket);
        m_hSocket = -1;
    }
    std::cerr << "Fail in client init" << std::endl;
    return -1;

}

int SimpleClient::Start()
{
    if(m_bWork)
        return -1;
    int ret;
    ret = ConnectHost();
    if(ret < 0)
        goto ERR;
    ret = m_Listener.RegisterRecevier(m_hSocket, this);
    if(ret < 0)
        goto ERR;
    ret = Login();
    if(ret == HANDLEMSGRESULT_LOGINAUTHSUCCESS)
    {
        m_bWork = true;
        while(m_bWork)
        {
            OnReceive();
        }
    }
    //remove..
    return 0;
    ERR:
    close(m_hSocket);
    m_hSocket = -1;
    return -1;
}

void SimpleClient::OnReceive()
{
    assert(this);
    SimpleMsgHdr *pMsg = Receive();
    if(!pMsg)
        return;
    int ret = HandleMsg(pMsg);
    if(ret < 0)
        m_bWork = false;

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
        std::cerr << "Connect to " << m_Port << " falied! " << strerror(errno) << std::endl;
        return ret;
    }

    return ret;
}

int SimpleClient::Login()
{
    assert(m_SelfAttr.UesrName.size() > 0);
    AuthInfo info{m_SelfAttr.UesrName};
    LoginMessage *pMsg = LoginMessage::Pack(m_SendBuf, info);
    PostMessage(m_hSocket, pMsg);
    return HANDLEMSGRESULT_LOGINAUTHSUCCESS;
}

SimpleMsgHdr *SimpleClient::Receive()
{
    int ret = RecevieMessage(m_hSocket, m_RecvBuf, BUF_MAX_LEN);
    if(ret < 0)
    {
        //..server close
        return NULL;
    }
    SimpleMsgHdr *pMsg = reinterpret_cast<SimpleMsgHdr*>(m_RecvBuf);
    assert(pMsg->FrameHead == MSG_FRAME_HEADER);
    return pMsg;
}

int SimpleClient::HandleMsg(SimpleMsgHdr *pMsg)
{
    return 0;
}
