#include <iostream>
#include <algorithm>
#include <sstream>

#include <cstring>
#include <errno.h>
#include <cstdio>

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "SimpleServer.h"
#include "Log/SimpleLogImpl.h"

static uint64_t sUID = 0;

SimpleServer::SimpleServer() : m_hListenFD(-1), m_Port(-1), m_bStart(false)
{

}

int SimpleServer::Init(int port)
{
    assert(this);
    assert(m_hListenFD < 0);

    m_Port = port;
    int ret;
    int on = 1;

    m_hListenFD = socket(AF_INET, SOCK_STREAM, 0);
    if(m_hListenFD < 0)
        goto ERR;

    ret = setsockopt(m_hListenFD, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(ret < 0)
        goto ERR;

    sockaddr_in  servAddr;
    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(m_Port);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(m_hListenFD, (sockaddr*)&servAddr, sizeof(servAddr));
    if(ret < 0)
    {
        DLOGERROR("bind port %d ERR: %s", m_Port, strerror(errno));
        goto ERR;
    }

    ret = listen(m_hListenFD, 20);
    if(ret < 0)
    {
        DLOGERROR("listen ERR: %s", strerror(errno));
        goto ERR;
    } 

    ret = m_Listener.InitListener();
    if(ret < 0)
    {
        DLOGERROR("m_Listener.InitListener");
        goto ERR;
    }
    ret = m_Listener.RegisterRecevier(m_hListenFD, this);
    if(ret < 0)
    {
        DLOGERROR("m_Listener.RegisterRecevier");
        ret = -4;
        goto ERR;
    }

    ret = pthread_mutex_init(&m_ReportMutex, NULL);
    if(ret < 0)
    {
        DLOGERROR("pthread_mutex_init %s", strerror(errno));
        ret = -4;
        goto ERR;
    }

    ret = pthread_cond_init(&m_ReportCond, NULL);
    if(ret < 0)
    {
        DLOGERROR("pthread_cond_init %s", strerror(errno));
        ret = -4;
        goto ERR;
    }

    return 0;

    ERR:
    if(m_hListenFD >= 0)
    {
        close(m_hListenFD);
        m_hListenFD = -1;
    }
    if(ret < -2)
        m_Listener.UninitListener();
    return ret;
}

int SimpleServer::Uninit()
{
    assert(this);
    int ret;
    if(m_hListenFD < 0)
    {
        DLOGWARN("Server not init!");
        return 0;
    }

    while(m_Sessions.size() > 0)
    {
        CSession *pSession = m_Sessions.back();
        assert(pSession);
        pSession->Destory();
        delete pSession;
        m_Sessions.pop_back();
    }

    ret = m_Listener.UnRegisterRecevier(m_hListenFD, this);
    if(ret < 0)
    {
        //error handle
        DLOGERROR("Failed to UnRegisterRecevier");

    }

    ret = m_Listener.UninitListener();
    if(ret < 0)
    {
        //error handle
        DLOGERROR("Failed to UninitListener");
    }

    ret = close(m_hListenFD);
    if(ret < 0)
    {
        DLOGERROR("Failed to close m_hListenFD %s", strerror(errno));
    }

    ret = pthread_mutex_destroy(&m_ReportMutex);
    if(ret < 0)
    {
        DLOGERROR("pthread_mutex_init %s", strerror(errno));
    }

    ret = pthread_cond_destroy(&m_ReportCond);
    if(ret < 0)
    {
        DLOGERROR("pthread_cond_init %s", strerror(errno));
    }


    m_hListenFD = -1;
    return ret;
}

int SimpleServer::Start()
{
    assert(this);
    assert(m_hListenFD >= 0);
    if(m_bStart)
    {
        DLOGWARN("Server has been started");
        return 0;
    }

    int ret = m_Listener.Start();
    if(ret < 0)
        goto ERR;
    m_bStart = true;

    return ret;
    ERR:
    DLOGERROR("Failed to start server!");
    return -1;
}

int SimpleServer::Stop()
{
    assert(this);
    int ret = 0;
    if(m_bStart)
    {
        ret = m_Listener.Stop();
        if(ret > 0)
            m_bStart = false;
    }
    return ret;
}

void SimpleServer::OnReceive()
{
    assert(this);
    assert(m_hListenFD >= 0);

    sockaddr_in cliAddr;
    socklen_t cliLen = sizeof(cliAddr);
    CSession *pSession;

    int cliFD = accept(m_hListenFD, (sockaddr *)&cliAddr, &cliLen);
    if(cliFD < 0)
        goto ERR;

    pSession = OnSessionCreate(cliFD);
    if(!pSession)
        goto ERR;

    return;

    ERR:
    if(cliFD >= 0)
    {
        close(cliFD);
    }
    else
    {
        DLOGERROR("Failed to accept! %s ", strerror(errno));
    }
}

int SimpleServer::RunServerLoop()
{
    while( m_bStart )
    {
        int ret = pthread_mutex_lock(&m_ReportMutex);
        if (ret < 0)
        {
            DLOGERROR("Failed Lock m_ReportMutex %s", strerror(errno));
        }
        while (m_Reports.empty())
        {
            ret = pthread_cond_wait(&m_ReportCond, &m_ReportMutex);
            if (ret < 0)
            {
                DLOGERROR("Failed wait m_ReportCond %s", strerror(errno));
            }
        }
        auto report = m_Reports.front();
        m_Reports.pop_front();
        ret = pthread_mutex_unlock(&m_ReportMutex);
        if (ret < 0)
        {
            DLOGERROR("Failed Lock m_ReportMutex %s", strerror(errno));
        }

        assert(report.pReporter);
        assert(report.pReport);
        (this->*report.pReport)(report.pReporter);
    }
}

void SimpleServer::PushToAll(const SimpleMsgHdr *pMsg)
{
    assert(pMsg);
    for(CSession *p : m_Sessions)
    {
        p->Send(pMsg);
    }
}

uint64_t SimpleServer::LoginAuthentication(const AuthInfo &info)
{
    //a lot of operation
    uint64_t uid = CheckNameExisted(info.UserName);
    if(uid != 0)
    {
        UserIt it = m_Users.find(uid);
        assert(it != m_Users.end());
        assert(it->first == it->second.UID );
        DLOGINFO("old in %s uid:% ld online:%d", info.UserName.c_str(), it->second.UID, it->second.bOnline);
        if(it->second.bOnline)
        {
            //this user has already online
            return 0;
        }
        else
        {
            it->second.bOnline = true;
            return uid;
        }
    }
    else
    {
        UserAttr attr;
        attr.UID = ++sUID;
        attr.bOnline = true;
        attr.UserName = info.UserName;
        m_Users.insert({attr.UID, attr});
        DLOGINFO("old in %s uid:% ld online:%d", attr.UserName.c_str(), attr.UID, attr.bOnline);

        return attr.UID;
    }

    return 0;
}

uint64_t SimpleServer::CheckNameExisted(const std::string &name) const
{
    UserList::const_iterator it = std::find_if(m_Users.cbegin(), m_Users.cend(), [&name](const UserList::value_type &pair)
    {
            return pair.second.UserName == name;
    });

    return it != m_Users.cend() ? it->second.UID : 0;
}

void SimpleServer::ReportMsgIn(CSession::SessionReport & report)
{
    pthread_mutex_lock(&m_ReportMutex);
    
    m_Reports.push_back(report);

    pthread_mutex_unlock(&m_ReportMutex);

    pthread_cond_signal(&m_ReportCond);
}

SimpleServer::CSession *SimpleServer::OnSessionCreate(int fd)
{
    CSession *pSession = new CSession(this, fd);
    int ret = 0;
    if(!pSession)
        goto ERR;
    ret = pSession->Create();
    if(ret < 0)
        goto ERR;
    m_Sessions.push_back(pSession);
    return pSession;

    ERR:
    DLOGERROR("Create session failed! ret %d", ret);
    if(pSession)
        delete pSession;
    return NULL;
}

void SimpleServer::OnSessionFinished(SimpleServer::CSession *pSession)
{
    assert(pSession);
    //erase session
    ListIt it = std::find(m_Sessions.begin(), m_Sessions.end(), pSession);
    assert(it != m_Sessions.end());
    m_Sessions.erase(it);

    //change status
    UserIt uit = m_Users.find( pSession->GetUID() );
    assert(uit != m_Users.end());
    uit->second.bOnline = false;
    pSession->Destory();
    delete pSession;
}

void SimpleServer::PrintAllUser()
{
    for(UserList::value_type pair : m_Users)
    {
        std::cout << "UID: " << pair.first << std::endl << "User name: " << pair.second.UserName << std::endl \
                  << "User online: " << std::boolalpha <<pair.second.bOnline << std::endl;
    }
}

SimpleServer::CSession::CSession(SimpleServer *pHost, int fd) : m_pServer(pHost),
    m_hFD(fd),  m_UID(0)
{
    assert(pHost);
    assert(fd >= 0);
}

SimpleServer::CSession::~CSession()
{
    assert(m_UID == 0);
}

void SimpleServer::CSession::OnReceive()
{
    assert(this);

    const SimpleMsgHdr *pMsg = Recevie();
    if(!pMsg)
    {
        SessionReport report;
        report.pReport = &SimpleServer::OnSessionFinished;
        report.pReporter = this;
        int ret = m_pServer->m_Listener.UnRegisterRecevier(m_hFD, this);
        if (ret < 0)
        {
            //error handle
            DLOGERROR("m_pServer->m_Listener.UnRegisterRecevier");
        }
        m_pServer->ReportMsgIn(report);
        
        return;
    }
    HandleMessage(pMsg);
    DestoryRecvMessage(pMsg);
}

int SimpleServer::CSession::Create()
{
    assert(this);
    assert(m_pServer);
    return m_pServer->m_Listener.RegisterRecevier(m_hFD, this);
}

int SimpleServer::CSession::Destory()
{
    assert(this);
    assert(m_pServer);

    ret = close(m_hFD);
    if(ret < 0)
    {
        DLOGERROR("close m_hFD %s", strerror(errno));
    }
    m_UID = 0;
    return ret;
}

const SimpleMsgHdr *SimpleServer::CSession::Recevie()
{
    SimpleMsgHdr *pMsg = NULL;
    int ret = RecevieMessage(m_hFD, &pMsg);
    if(ret < 0)
    {
        DLOGWARN("recv NULL msg");
        return NULL;
    }
    assert(pMsg->FrameHead == MSG_FRAME_HEADER);
    return pMsg;
}

int SimpleServer::CSession::HandleMessage(const SimpleMsgHdr *pMsg)
{
    assert(pMsg && m_pServer);
    int ret;

    switch(pMsg->ID)
    {
    case SPLMSG_LOGIN:
    {
        assert(m_UID == 0);
        AuthInfo info;
        LoginMessage::Unpack(reinterpret_cast<const SimpleMsgHdr*>(pMsg), info);
        assert(info.UserName.length() > 0);
        std::cout << info.UserName << " request login!" << std::endl;
        uint64_t uid= m_pServer->LoginAuthentication(info);
        if(uid == 0)
        {
            std::cout << info.UserName << " has existed, send ERR msg!" << std::endl;
            ErrMessage errmsg(ERRMSG_NAMEXIST);
            ret = Send(&errmsg);
        }
        else
        {
            //一条单独的SPLMSG_OK
            m_UID = uid;
            UserIt it = m_pServer->m_Users.find(m_UID);
            assert(it != m_pServer->m_Users.end());
            LoginOkMessage pLoginOk(uid);
            ret = Send(&pLoginOk);
            if(ret < 0)
                return -1;

            //有新的登录后应该发一条消息通知所有人，假如是新加入用户登录则发DataMsg->UserAttrMsg，更新所有在线客户端缓存
            //如果是老用户加入，则发另一条消息告诉所有人把他的online置位
            UserAttrMsg *pAttrMsg = UserAttrMsg::Create(m_pServer->m_Users.begin(), m_pServer->m_Users.end());
            Send(pAttrMsg);
            UserAttrMsg::Destory(pAttrMsg);

            UserAttr attr = it->second;
            LoginNoticeMsg *pNotice = LoginNoticeMsg::Pack(m_SendBuf, attr);
            m_pServer->PushToAll(pNotice);
        }
    }
        break;
    case SPLMSG_USERINPUT:
    {
        ClientText cText;
        UserInputMsg::Unpack(pMsg, cText);
        ServerText sText{time(NULL), cText.UID, cText.content};
        SimpleMsgHdr *pChat = ServerChatMsg::Pack(m_SendBuf, sText);
        m_pServer->m_Records.push_back(sText);
        m_pServer->PushToAll(pChat);
    }
        break;
    default:
        std::cout << "Server session recv Unkown Message Type: " << std::hex << pMsg->ID <<std::endl;
        break;

    }
    return ret;
}
