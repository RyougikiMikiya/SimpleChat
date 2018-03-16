
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
    assert(this);
    assert(m_hSocket == -1);
    assert(pName);
    assert(pIP);
    int ret;
    std::string name(pName);
    m_Port = port;
    m_SelfAttr.UesrName = name;
    int on = 1;

    ret = inet_pton(AF_INET, pIP, &m_HostIP);
    if(ret != 1)
    {
        std::cout << "Invaild IP address!Please enter IPV4 addr!" << std::endl;
        goto ERR;
    }
    m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_hSocket < 0)
        goto ERR;

    ret = setsockopt(m_hSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(ret < 0)
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
    assert(!m_bWork);
    int ret;
    ret = ConnectHost();
    if(ret < 0)
        goto ERR;
    ret = m_Listener.RegisterRecevier(m_hSocket, this);
    if(ret < 0)
        goto ERR;
    ret = Login();
    if(ret == 0)
    {
        m_bWork = true;
        while(m_bWork)
        {
            OnReceive();
        }
    }
    else
    {
        std::cout << "Login failed" << std::endl;
    }
    //remove..
    return 0;
    ERR:
    std::cout << "Start client failed" << std::endl;
    close(m_hSocket);
    m_hSocket = -1;
    return -1;
}

void SimpleClient::OnReceive()
{
    assert(this);
    const SimpleMsgHdr *pMsg = Receive();
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
    servaddr.sin_addr.s_addr = m_HostIP;

    assert(m_hSocket >= 0);

    ret = connect(m_hSocket, (sockaddr *)&servaddr, sizeof(servaddr));
    if(ret < 0)
    {
        std::cerr << "Connect to " << m_Port << " " << m_HostIP <<" falied! " << strerror(errno) << std::endl;
        return ret;
    }

    return ret;
}

int SimpleClient::Login()
{
    assert(m_SelfAttr.UesrName.size() > 0);
    AuthInfo info{m_SelfAttr.UesrName};
    LoginMessage *pMsg = LoginMessage::Pack(m_SendBuf, info);
    int ret = PostMessage(m_hSocket, pMsg);
    if(ret < 0)
    {

    }
    const SimpleMsgHdr *pResp = Receive();
    ret = HandleMsg(pResp);
    if(ret != HANDLEMSGRESULT_LOGINAUTHSUCCESS)
        return -1;
    return 0;
}

const SimpleMsgHdr *SimpleClient::Receive()
{
    int ret = RecevieMessage(m_hSocket, m_RecvBuf, BUF_MAX_LEN);
    if(ret < 0)
    {
        //..server close
        return NULL;
    }
    const SimpleMsgHdr *pMsg = reinterpret_cast<const SimpleMsgHdr*>(m_RecvBuf);
    assert(pMsg->FrameHead == MSG_FRAME_HEADER);
    return pMsg;
}

int SimpleClient::HandleMsg(const SimpleMsgHdr *pMsg)
{
    assert(pMsg);
    assert(pMsg->FrameHead == MSG_FRAME_HEADER);
    switch(pMsg->ID)
    {
    case SPLMSG_LOGIN_OK:
        return HANDLEMSGRESULT_LOGINAUTHSUCCESS;
    case SPLMSG_CHAT:
    {
        PutOutMsg(pMsg);
    }
        break;
    case SPLMSG_ERR:
    {

    }
        break;
    default:
        std::cout << "Unknown Message type!" << std::endl;
    }
    return 0;
}
