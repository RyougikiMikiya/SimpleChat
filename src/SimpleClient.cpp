
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

SimpleClient::SimpleClient() : m_hSocket(-1), m_Port(-1), m_HostIP(0), m_bWork(false), m_pUI(nullptr), m_cliStatus(CLIENTSTATUS_NEED_LOGIN)
{
}

SimpleClient::~SimpleClient()
{
    assert(m_hSocket == -1);
    assert(!m_pUI);
    assert(!m_bWork);
}

int SimpleClient::Init(const char *pIP, int port)
{
    assert(this);
    assert(m_hSocket == -1);
    assert(pIP);
    int ret;
    m_strIP = pIP;
    m_Port = port;

    ret = inet_pton(AF_INET, pIP, &m_HostIP);
    if (ret != 1)
    {
        DLOGERROR("Invaild IP address %s", pIP);
        abort();
        goto mERR;
    }

    ret = m_Listener.InitListener();
    if (ret < 0)
    {
        ret = -2;
        goto mERR;
    }
    ret = m_Listener.Start();
    ret = pthread_mutex_init(&m_queMutex, NULL);

    ret = pthread_cond_init(&m_Cond, NULL);

    ret = sem_init(&m_LoginSem, 0, 0);
    ret = sem_init(&m_LogoutSem, 0, 0);
    //启动ui
    m_pUI = new SimpleUI(this);
    if (!m_pUI)
    {
        goto mERR;
    }
    ret = m_pUI->Run();
    if (ret < 0)
    {
        ret = -3;
        goto mERR;
    }

    return 0;

mERR:
    DLOGERROR("Init client err %d", ret);
    return -1;
}

int SimpleClient::Run()
{
    assert(this);
    assert(!m_bWork);
    int ret;
    m_bWork = true;
    while (m_bWork)
    {
        //client主线程lock
        DLOGDEBUG("Client main thread enter lock");
        pthread_mutex_lock(&m_queMutex);
        while (m_EventQueue.empty())
        {
            pthread_cond_wait(&m_Cond, &m_queMutex);
        }
        UIevent event = m_EventQueue.front();
        m_EventQueue.pop_front();
        pthread_mutex_unlock(&m_queMutex);
        DLOGDEBUG("Client main thread leave lock");
        switch (event.eventType)
        {
        case UI_USER_LOGIN:
        {
            assert(event.arg.type() == typeid(std::string));
            assert(event.arg.has_value());
            m_SelfAttr.UserName = std::any_cast<std::string>(event.arg);
            if (m_hSocket < 0)
            {
                ret = ConnectHost();
                if (ret < 0)
                {
                    //  return net err to ui
                }
                ret = m_Listener.RegisterRecevier(m_hSocket, this);
            }
            ret = Login();
            if (ret < 0)
            {
                //error when send log info
            }
        }
        break;
        case UI_USER_INPUT:
        {
            assert(event.arg.type() == typeid(std::string));
            assert(event.arg.has_value());
            ClientText text{m_SelfAttr.UID, std::any_cast<std::string>(event.arg)};
            SimpleMsgHdr *pMsg = UserInputMsg::Pack(m_SendBuf, text);
            int ret = Send(pMsg);
            if (ret < 0)
            {
                DLOGERROR("Client failed to Send msg");
            }
            DLOGDEBUG("send usr input from client");
        }
        break;
        case UI_USER_LOGOUT:
        {
            DLOGDEBUG("recv user logout");
            SimpleMsgHdr *pMsg = LogoutMsg::Pack(m_SendBuf, m_SelfAttr);
            int ret = Send(pMsg);
            if (ret < 0)
            {
                DLOGERROR("Client failed to Send msg");
                m_SelfAttr.clear(); 
                m_cliStatus = CLIENTSTATUS_NEED_LOGIN;
                sem_post(&m_LogoutSem);
            }
        }
        break;
        case UI_USER_QUIT:
        {
            DLOGDEBUG("recv user quit");
            m_pUI->WaitUIStop();
            Stop();
            m_bWork = false;
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
    int ret = m_Listener.UnRegisterRecevier(m_hSocket, this);
    ret = m_Listener.Stop();
    ret = m_Listener.UninitListener();
    ret = close(m_hSocket);
    if (ret < 0)
        DLOGERROR("close client socket err");
    m_hSocket = -1;
    delete m_pUI;
    m_pUI = nullptr;
    pthread_mutex_destroy(&m_queMutex);
    pthread_cond_destroy(&m_Cond);
    return ret;
}

void SimpleClient::OnReceive()
{
    assert(this);
    const SimpleMsgHdr *pMsg = Receive();
    if (!pMsg)
    {
        m_Listener.UnRegisterRecevier(m_hSocket, this);
        m_bWork = false;
        return;
    }
    int ret = HandleMsg(pMsg);
    DestoryRecvMessage(pMsg);
    if (ret < 0)
        m_bWork = false;
}

void SimpleClient::putUIevent(UIevent &event)
{
    DLOGDEBUG("Enter a lock");
    pthread_mutex_lock(&m_queMutex);
    m_EventQueue.push_back(event);
    sem_destroy(&m_LoginSem);
    sem_destroy(&m_LogoutSem);
    pthread_mutex_unlock(&m_queMutex);
    pthread_cond_signal(&m_Cond);
    DLOGDEBUG("Leave a lock");
}

SimpleClient::CLIENTSTATUS SimpleClient::GetCliStatus()
{
    return m_cliStatus;
}

void SimpleClient::WaitLoginFinish()
{
    sem_wait(&m_LoginSem);
}

void SimpleClient::WaitLogOutFinish()
{
    sem_wait(&m_LogoutSem);
}

int SimpleClient::ConnectHost()
{
    int ret;

    int on = 1;
    m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_hSocket < 0)
    {
        DLOGERROR("create socket failed");
        return -1;
    }

    ret = setsockopt(m_hSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret < 0)
    {
        return -1;
    }

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(m_Port);
    servaddr.sin_addr.s_addr = m_HostIP;

    assert(m_hSocket >= 0);

    ret = connect(m_hSocket, (sockaddr *)&servaddr, sizeof(servaddr));
    if (ret < 0)
    {
        DLOGERROR("connect %s %d error %d: %s", m_strIP.c_str(), m_Port, errno, strerror(errno));
        close(m_hSocket);
        m_hSocket = -1;
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
    if (ret < 0)
    {
        DLOGERROR("send logmsg failed");
        return -1;
    }
    return ret;
}

std::string SimpleClient::FormatClientText(const ServerText &text) const
{
    UserConstIt it = m_Users.find(text.UID);
    assert(it != m_Users.cend());
    std::stringstream stream;
    struct tm *pTime = localtime(&text.Time);
    stream << "<" << pTime->tm_hour << ">:"
           << '<' << pTime->tm_min << ">:"
           << '<' << pTime->tm_sec << ">"
           << it->second.UserName << ": "
           << text.content
           << std::endl;
    return stream.str();
}

const SimpleMsgHdr *SimpleClient::Receive()
{
    SimpleMsgHdr *pMsg;
    int ret = RecevieMessage(m_hSocket, &pMsg);
    if (ret < 0)
    {
        //..server close
        return NULL;
    }
    assert(pMsg->FrameHead == MSG_FRAME_HEADER);
    return pMsg;
}

SimpleClient::CLI_MSG_RESULT SimpleClient::HandleMsg(const SimpleMsgHdr *pMsg)
{
    assert(pMsg);
    assert(pMsg->FrameHead == MSG_FRAME_HEADER);
    switch (pMsg->ID)
    {
    case SPLMSG_LOGIN_OK:
    {
        DLOGINFO("Recv SPLMSG_LOGIN_OK");
        m_cliStatus = CLIENTSTATUS_LOGIN_SUCCESS;
        m_pUI->PrintToScreen("Login success!");
        sem_post(&m_LoginSem);
        const LoginOkMessage *pLogin = static_cast<const LoginOkMessage *>(pMsg);
        assert(pLogin->UID != 0);
        m_SelfAttr.UID = pLogin->UID;
    }
    break;
    case SPLMSG_LOGIN_NOTICE:
    {
        DLOGINFO("Recv SPLMSG_LOGIN_NOTICE");
        UserAttr attr;
        LoginNoticeMsg::Unpack(pMsg, attr);
        UserIt it = m_Users.find(attr.UID);
        if(m_Users.end() == it)
        {
            m_Users.insert({attr.UID, attr});
        }
        else
        {
            it->second.bOnline = true;
        }
        if (m_SelfAttr.UID == attr.UID)
            break;
        std::stringstream stream;
        stream << "Welcome " << attr.UserName << " come in!" << std::endl;
        m_pUI->PrintToScreen(stream.str().c_str());
    }
    break;
    case SPLMSG_LOGOUT_NOTICE:
    {
        DLOGINFO("Recv SPLMSG_LOGOUT_NOTICE");
        UserAttr attr;
        LogoutNoticeMsg::Unpack(pMsg, attr);
        UserIt it = m_Users.find(attr.UID);
        assert(it != m_Users.end());
        it->second.bOnline = false;
        if( m_SelfAttr.UID == attr.UID )
        {
            m_SelfAttr.clear();
            m_cliStatus = CLIENTSTATUS_NEED_LOGIN;
            sem_post(&m_LogoutSem);
        }
        else
        {
            std::stringstream stream;
            stream << attr.UserName << " has logged out!" << std::endl;
            m_pUI->PrintToScreen(stream.str().c_str());
        }
    }
    break;
    case SPLMSG_CHAT:
    {
        DLOGINFO("Recv SPLMSG_CHAT");
        ServerText text;
        ServerChatMsg::Unpack(pMsg, text);
        m_pUI->PrintToScreen(FormatClientText(text).c_str());
    }
    break;
    case SPLMSG_ERR:
    {
        ErrMessage *pErrMsg = (ErrMessage *)pMsg;
        DLOGWARN("recv error msg type: %04x", pErrMsg->Errtype);
        m_pUI->PrintToScreen("Err! Err type :");
        switch (pErrMsg->Errtype)
        {
        case ERRMSG_NAMEXIST:
        {
            //need more opreation
            m_cliStatus = CLIENTSTATUS_NEED_LOGIN;
            m_pUI->PrintToScreen("This name has already logined!");
            sem_post(&m_LoginSem);
        }
        break;

        default:
            break;
        }
    }
    break;
    case SPLMSG_USERATTR:
    {
        DLOGINFO("Recv SPLMSG_USERATTR");
        UserAttrMsg::Unpack(static_cast<const UserAttrMsg *>(pMsg), m_Users);
    }
    break;
    default:
        std::cout << "Client recv Unknown Message type!" << std::endl;
    }
    return CMSGR_NORMAL;
}
