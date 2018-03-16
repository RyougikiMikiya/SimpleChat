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

static uint64_t sUID = 0;

SimpleServer::SimpleServer() : m_hListenFD(-1), m_Port(-1), m_bStart(false), m_STDIN(this)
{

}

int SimpleServer::Init(const char *pName, int port)
{
    assert(this);
    assert(pName);
    assert(m_hListenFD < 0);
    m_SelfAttr.UesrName = pName;
    assert(!m_SelfAttr.UesrName.empty());
    m_SelfAttr.UID = sUID++;
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
        goto ERR;

    ret = listen(m_hListenFD, 20);
    if(ret < 0)
        goto ERR;

    ret = m_Listener.InitListener();
    if(ret < 0)
    {
        ret = -2;
        goto ERR;
    }
    ret = m_Listener.RegisterRecevier(STDIN_FILENO, &m_STDIN);
    if(ret < 0)
    {
        ret = -3;
        goto ERR;
    }
    ret = m_Listener.RegisterRecevier(m_hListenFD, this);
    if(ret < 0)
    {
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

    if(ret == -4)
        m_Listener.UnRegisterRecevier(STDIN_FILENO, &m_STDIN);
    sUID = 0;
    std::cerr << "Server Init ERR: " << strerror(errno) << std::endl;
    return ret;
}

int SimpleServer::Uninit()
{
    assert(this);
    int ret;
    if(m_hListenFD < 0)
    {
        std::cout << "Server not init!" << std::endl;
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

    ret = m_Listener.UnRegisterRecevier(STDIN_FILENO, &m_STDIN);
    assert( ret >= 0);
    ret = m_Listener.UnRegisterRecevier(m_hListenFD, this);
    assert( ret >= 0);
    ret = m_Listener.UninitListener();
    if(ret < 0)
        std::cout << "Listener Uninit fail" << std::endl;
    ret = close(m_hListenFD);
    if(ret < 0)
    {
        std::cerr << "Server Uninit ERR: " << strerror(errno) << std::endl;
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
        std::cout << "Server has been started" << std::endl;
        return 0;
    }

    int ret = m_Listener.Start();
    if(ret < 0)
        goto ERR;
    m_bStart = true;

    return ret;
    ERR:
    std::cerr << "Failed to start server!" << std::endl;
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
        perror("Failed to accept!");
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
    assert(sUID != 0);

    //a lot of operation
    if(info.UserName == m_SelfAttr.UesrName)
        return 0;

    uint64_t uid = CheckNameExisted(info.UserName);
    if(uid != 0)
    {
        std::cout << "old in" << std::endl;
        UserIt it = m_Users.find(uid);
        assert(it != m_Users.end());
        assert(!it->second.bOnline);
        it->second.bOnline = true;
        return uid;
    }
    else
    {
        std::cout << "new in" << std::endl;
        UserAttr attr;
        attr.UID = sUID;
        sUID++;
        attr.bOnline = true;
        attr.UesrName = info.UserName;
        m_Users.insert({attr.UID, attr});
        return attr.UID;
    }

    return 0;
}



uint64_t SimpleServer::CheckNameExisted(const std::string &name) const
{
    UserList::const_iterator it = std::find_if(m_Users.cbegin(), m_Users.cend(), [&name](const UserList::value_type &pair)
    {
            return pair.second.UesrName == name;
    });

    return it != m_Users.cend() ? it->second.UID : 0;
}

SimpleServer::CSession *SimpleServer::OnSessionCreate(int fd)
{
    CSession *pSession = new CSession(this, fd);
    int ret;
    if(!pSession)
        goto ERR;
    ret = pSession->Create();
    if(ret < 0)
        goto ERR;
    m_Sessions.push_back(pSession);
    return pSession;

    ERR:
    std::cerr << "Create session failed!" << std::endl;
    if(pSession)
        delete pSession;
    return NULL;
}

void SimpleServer::OnSessionFinished(SimpleServer::CSession *pSession)
{
    assert(pSession);
    ListIt it = std::find(m_Sessions.begin(), m_Sessions.end(), pSession);
    assert(it != m_Sessions.end());
    m_Sessions.erase(it);
    //return value???
    uint64_t uid = pSession->GetUID();
    if(uid != 0)
    {
        UserIt it = m_Users.find(uid);
        assert(it != m_Users.end());
        it->second.bOnline = false;
    }
    pSession->Destory();
    delete pSession;
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
        return;
    }
    HandleMessage(pMsg);
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

    if(m_UID != 0)
        m_UID = 0;
    int ret = m_pServer->m_Listener.UnRegisterRecevier(m_hFD, this);
    if(ret < 0)
        goto ERR;
    if(m_hFD < 0)
        return 0;
    ret = close(m_hFD);
    if(ret < 0)
        goto ERR;
    return ret;

    ERR:
    std::cerr << "Failed to destory sesssion " << std::endl;
    return ret;
}

const SimpleMsgHdr *SimpleServer::CSession::Recevie()
{
    int ret = RecevieMessage(m_hFD, m_RecvBuf, BUF_MAX_LEN);
    if(ret < 0)
    {
        m_pServer->OnSessionFinished(this);
        return NULL;
    }

    const SimpleMsgHdr *pMsg = reinterpret_cast<const SimpleMsgHdr*>(m_RecvBuf);
    assert(pMsg->FrameHead == MSG_FRAME_HEADER);
    return pMsg;
}

int SimpleServer::CSession::HandleMessage(const SimpleMsgHdr *pMsg)
{
    assert(pMsg && m_pServer);
    int ret;
    std::stringstream contents;
    int length;

    switch(pMsg->ID)
    {
    case SPLMSG_LOGIN:
    {
        assert(m_UID == 0);
        AuthInfo info;
        LoginMessage::Unpack(reinterpret_cast<const SimpleMsgHdr*>(m_RecvBuf), info);
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
            //一条单独的SPLMSG_OK，一条群发welcome
            m_UID = uid;
            assert(!m_pServer->m_SelfAttr.UesrName.empty());
            contents << m_pServer->m_SelfAttr.UesrName;
            length = contents.str().size();
            LoginOkMessage *pLoginOk = new(m_SendBuf) LoginOkMessage(length);
            memcpy(pLoginOk->HostName, contents.str().c_str(), length);
            ret = Send(pLoginOk);
            pLoginOk->~LoginOkMessage();
            if(ret < 0)
                return -1;

            contents.clear();
            contents.str("");

            contents << "Welcome " << info.UserName << " add in!";
            length = contents.str().size();

            ChatMessage *pWelcome = new(m_SendBuf) ChatMessage(length, m_UID);
            memcpy(pWelcome->Contents, contents.str().c_str(), length);
            PutOutMsg(pWelcome);
            m_pServer->PushToAll(pWelcome);
            pWelcome->~ChatMessage();
        }
    }
        break;
    case SPLMSG_CHAT:
    {
        const ChatMessage *pText = static_cast<const ChatMessage*>(pMsg);
        PutOutMsg(pText);
        m_pServer->PushToAll(pText);
    }
        break;
    default:
        std::cout << "Unkown Message Type!" << std::endl;
        break;

    }
    return ret;
}

void SimpleServer::SInputReceiver::OnReceive()
{
    assert(this);
    assert(m_pServer);
    std::getline(std::cin, m_LineBuf);
    assert(m_pServer->m_SelfAttr.UID == 0);
    ChatMessage *pMsg = new(m_SendBuf) ChatMessage(m_LineBuf.size(), m_pServer->m_SelfAttr.UID);
    memcpy(pMsg->Contents, m_LineBuf.c_str(), m_LineBuf.size());
    PutOutMsg(pMsg);
    m_pServer->PushToAll(pMsg);
}
