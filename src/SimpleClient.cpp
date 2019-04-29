
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
#include "Log/SimpleLogImpl.h"

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
    m_strIP = pIP;
    m_Port = port;
    m_SelfAttr.UserName = name;
    int on = 1;

    ret = inet_pton(AF_INET, pIP, &m_HostIP);
    if(ret != 1)
    {
        DLOGERROR("Invaild IP address %s", pIP);
        goto mERR;
    }
    m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_hSocket < 0)
        goto mERR;

    ret = setsockopt(m_hSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(ret < 0)
        goto mERR;

    ret = m_Listener.InitListener();
    if(ret < 0)
    {
        ret = -2;
        goto mERR;
    }
    ret = pthread_mutex_init(&m_queMutex, NULL);

    ret = pthread_cond_init(&m_Cond, NULL);

    //启动ui
    ret = m_pUI->Run();
    if(ret < 0)
    {
        ret = -3;
        goto mERR;
    }

    return 0;

    mERR:
    if(m_hSocket >=0)
    {
        close(m_hSocket);
        m_hSocket = -1;
    }
    DLOGERROR("Init client err %d", ret);
    return -1;

}

int SimpleClient::Run()
{
    assert(!m_bWork);
    int ret;
    //启动网络部分
    ret = ConnectHost();
    if(ret < 0)
        goto mERR;

    ret = m_Listener.RegisterRecevier(m_hSocket, this);
    if(ret < 0)
        goto mERR;

    ret = Login();
    if(ret == 0)
    {
        m_Listener.Start();
    }
    else
    {
        std::cout << "Login failed" << std::endl;
    }

    m_bWork = true;
    while (m_bWork)
    {
        //client主线程lock
        pthread_mutex_lock(&m_queMutex);
        while(m_EventQueue.empty())
        {
            pthread_cond_wait(&m_Cond, &m_queMutex);
        }
        UIevent event = m_EventQueue.front();
        m_EventQueue.pop_front();
        pthread_mutex_unlock(&m_queMutex);
        switch (event.eventType)
        {
        case UI_USER_INPUT:
        {
            assert( event.arg.type() == typeid(std::string) );
            assert( event.arg.has_value());
            ClientText text{m_SelfAttr.UID, std::any_cast<std::string>(event.arg)};
            SimpleMsgHdr *pMsg = UserInputMsg::Pack(m_SendBuf, text);
            Send(pMsg);
        }
            break;
        
        default:
            break;
        }
    }
    
    return 0;
mERR:
    std::cout << "Start client failed" << std::endl;
    close(m_hSocket);
    m_hSocket = -1;
    return -1;
}

int SimpleClient::Stop()
{
    assert(this);
    return m_Listener.Stop();
}

void SimpleClient::OnReceive()
{
    assert(this);
    const SimpleMsgHdr *pMsg = Receive();
    if(!pMsg)
    {
        m_Listener.UnRegisterRecevier(m_hSocket, this);
        m_bWork = false;
        return;
    }
    int ret = HandleMsg(pMsg);
    DestoryRecvMessage(pMsg);
    if(ret < 0)
        m_bWork = false;
}

void SimpleClient::putUIevent(UIevent & event)
{
    pthread_mutex_lock(&m_queMutex);
    m_EventQueue.push_back(event);
    pthread_mutex_unlock(&m_queMutex);
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
        DLOGERROR("connect %s %d error %d: %s", m_strIP.c_str(), m_Port, errno, strerror(errno));
        return ret;
    }

    return ret;
}

int SimpleClient::Login()
{
    assert(m_SelfAttr.UserName.size() > 0);
    AuthInfo info{m_SelfAttr.UserName};
    LoginMessage *pLoginMsg = LoginMessage::Pack(m_SendBuf, info);
    int ret = Send(pLoginMsg);
    if(ret < 0)
    {

    }
    const SimpleMsgHdr *pResp = Receive();
    if(!pResp)
    {
        return -1;
    }
    ret = HandleMsg(pResp);
    DestoryRecvMessage(pResp);
    if(ret != HANDLEMSGRESULT_LOGINAUTHSUCCESS)
        return -1;
    return 0;
}

void SimpleClient::PrintMsgToScreen(const SimpleMsgHdr *pMsg)
{
    assert(pMsg);
    assert(pMsg->FrameHead == MSG_FRAME_HEADER);
    assert(pMsg->Length < BUF_MAX_LEN);

    switch(pMsg->ID)
    {
    case SPLMSG_CHAT :
    {
        ServerText text;
        ServerChatMsg::Unpack(pMsg, text);
        m_pUI->PrintToScreen( FormatClientText(text).c_str());
    }
    case SPLMSG_ERR:
    {
        ErrMessage *pErrMsg = (ErrMessage *)pMsg;
        m_pUI->PrintToScreen("Err! Err type :");
    }
        break;
    default:
        std::cout << "Unkown Message type" << std::endl;
        break;
    }
}

std::string SimpleClient::FormatClientText(const ServerText &text) const
{
    UserConstIt it = m_Users.find(text.UID);
    assert(it != m_Users.cend());
    std::stringstream stream;
    struct tm *pTime = localtime(&text.Time);
    stream << "<" <<pTime->tm_hour << ">:"
            << '<' <<pTime->tm_min << ">."
            << '<' <<pTime->tm_sec << ">"
            << it->second.UserName << ": "
            << text.content
            << std::endl;
    return stream.str();
}

const SimpleMsgHdr *SimpleClient::Receive()
{
    SimpleMsgHdr *pMsg;
    int ret = RecevieMessage(m_hSocket, &pMsg);
    if(ret < 0)
    {
        //..server close
        return NULL;
    }
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
    {
        std::cout << "Login success!" << std::endl;
        const LoginOkMessage *pLogin = static_cast<const LoginOkMessage*>(pMsg);
        assert(pLogin->UID != 0);
        m_SelfAttr.UID = pLogin->UID;
        return HANDLEMSGRESULT_LOGINAUTHSUCCESS;
    }
        break;
    case SPLMSG_LOGIN_NOTICE:
    {
        UserAttr attr;
        LoginNoticeMsg::Unpack(pMsg, attr);
        if(attr.UID == m_SelfAttr.UID)
            break;
        m_Users.insert({attr.UID, attr});
    }
        break;
    case SPLMSG_CHAT:
    {
        PrintMsgToScreen(pMsg);
    }
        break;
    case SPLMSG_ERR:
    {
        PrintMsgToScreen(pMsg);
    }
        break;
    case SPLMSG_USERATTR:
    {
        UserAttrMsg::Unpack(static_cast<const UserAttrMsg*>(pMsg), m_Users);
    }
        break;
    default:
        std::cout << "Client recv Unknown Message type!" << std::endl;
    }
    return 0;
}
