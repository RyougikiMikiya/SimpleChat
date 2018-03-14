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

SimpleServer::SimpleServer() : m_hListenFD(-1), m_Port(-1), m_bStart(false)
{

}

int SimpleServer::Init(const char *pName, int port)
{
    assert(this);
    assert(m_hListenFD < 0);
    m_selfAttr.UesrName = pName;
    assert(!m_selfAttr.UesrName.empty());
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
        goto ERR;

    ret = m_Listener.RegisterRecevier(m_hListenFD, this);
    if(ret < 0)
        goto ERR;

    return 0;

    ERR:
    if(m_hListenFD >= 0)
    {
        close(m_hListenFD);
        m_hListenFD = -1;
    }
    std::cerr << "Server Init ERR: " << strerror(errno) << std::endl;
    return ret;
}

int SimpleServer::Uninit()
{
    int ret;
    if(m_hListenFD < 0)
    {
        std::cout << "Server not init!" << std::endl;
        return 0;
    }

    for(SessionList::reverse_iterator rit = m_Sessions.rbegin(); rit!= m_Sessions.rend(); )
    {
        assert(*rit);
        (*rit)->Destory();
        delete (*rit);
        m_Sessions.erase((++rit).base());
    }

    ret = m_Listener.UnRegisterRecevier(m_hListenFD, this);
    if(ret < 0)
        ;
    ret = m_Listener.UninitListener();
    if(ret < 0)
        std::cout << "Listener Uninit fail" << std::endl;
    ret = close(m_hListenFD);
    if(ret < 0)
    {
        std::cerr << "Server Uninit ERR: " << strerror(errno) << std::endl;
        return ret;
    }
    m_hListenFD = -1;
    return ret;
}

int SimpleServer::Start()
{
    assert(this);
    assert(m_hListenFD >= 0);
    int ret = m_Listener.Start();
    if(ret < 0)
        goto ERR;

    return ret;
    ERR:
    std::cerr << "Failed to start server!" << std::endl;
    return -1;
}

int SimpleServer::Stop()
{
    return m_Listener.Stop();
}

void SimpleServer::OnReceive()
{
    assert(this);
    assert(m_hListenFD >= 0);

    sockaddr_in cliAddr;
    socklen_t cliLen;
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

void SimpleServer::PushToAll(SimpleMsgHdr *pMsg)
{
    for(CSession *p : m_Sessions)
    {
        p->SessionPost(pMsg);
    }
}

bool SimpleServer::CheckNameExisted(const std::string &name)
{
    ListIt it = std::find_if(m_Sessions.begin(), m_Sessions.end(), [&name](CSession *pSession)
    {
            return pSession->GetUserName() == name;
    });
    return it != m_Sessions.end() && name != m_selfAttr.UesrName ? true : false;
}

void SimpleServer::DisplayMsg(SimpleMsgHdr *pMsg)
{
    assert(pMsg);
    assert(pMsg->ID == SPLMSG_CHAT);
    assert(pMsg->Length < 2048);
    ChatMessage *pFull = static_cast<ChatMessage*>(pMsg);
    pFull->Contents[pMsg->Length] = '\0';
    std::cout << pFull->Contents << std::endl;
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
    if(it == m_Sessions.end())
    {
        std::cerr << "No this session in host!" << std::endl;
        return;
    }
    m_Sessions.erase(it);
    //return value???
    pSession->Destory();
    delete pSession;
}

SimpleServer::CSession::CSession(SimpleServer *pHost, int fd) : m_pServer(pHost),
    m_hFD(fd)
{
    assert(pHost);
    assert(fd >= 0);
}

SimpleServer::CSession::~CSession()
{

}

void SimpleServer::CSession::OnReceive()
{
    SimpleMsgHdr *pMsg = SessionRecv();
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
    int ret = m_pServer->m_Listener.UnRegisterRecevier(m_hFD, this);
    if(ret < 0)
        goto ERR;
    ret = close(m_hFD);
    if(ret < 0)
        goto ERR;
    return ret;

    ERR:
    std::cerr << "Failed to destory sesssion " << std::endl;
    return ret;
}

SimpleMsgHdr *SimpleServer::CSession::SessionRecv()
{
    int ret = RecevieMessage(m_hFD, m_RecvBuf, 2048);
    if(ret == 0)
        return NULL;
    if(ret < 0)
        m_pServer->OnSessionFinished(this);

    SimpleMsgHdr *pMsg = reinterpret_cast<SimpleMsgHdr*>(m_RecvBuf);
    assert(pMsg->FrameHead == MSG_FRAME_HEADER);
    return pMsg;
}

int SimpleServer::CSession::HandleMessage(SimpleMsgHdr *pMsg)
{
    assert(pMsg && m_pServer);
    int ret;
    std::stringstream contents;
    int length;

    switch(pMsg->ID)
    {
    case SPLMSG_LOGIN:
    {
        LoginMessage *pLoginMsg = reinterpret_cast<LoginMessage *>(pMsg);
        std::string name(pLoginMsg->LoginInfo);
        AuthInfo info{name};
        std::cout << name << " request login!" << std::endl;
        if(this->LoginAuthentication(info))
        {
            std::cout << name << " has existed, send ERR msg!" << std::endl;
            ErrMessage errmsg(ERRMSG_NAMEXIST);
            ret = SessionPost(&errmsg);
        }
        else
        {
            //一条单独的SPLMSG_OK，一条群发welcome
            assert(!m_pServer->m_selfAttr.UesrName.empty());
            contents << m_pServer->m_selfAttr.UesrName;
            length = contents.str().size();
            LoginOkMessage *pLoginOk = new(m_SendBuf) LoginOkMessage(length);
            memcpy(pLoginOk->HostName, contents.str().c_str(), length);
            ret = SessionPost(pLoginOk);
            pLoginOk->~LoginOkMessage();
            if(ret < 0)
                return -1;

            contents.clear();
            contents.str("");

            contents << "Welcome " << name << " add in!";
            length = contents.str().size();

            ChatMessage *pWelcome = new(m_SendBuf) ChatMessage(length);
            memcpy(pWelcome->Contents, contents.str().c_str(), length);
            m_pServer->DisplayMsg(pWelcome);
            m_pServer->PushToAll(pWelcome);
            pWelcome->~ChatMessage();
        }
    }
        break;
    case SPLMSG_CHAT:
    {
        ChatMessage *pText = static_cast<ChatMessage*>(pMsg);
        m_pServer->DisplayMsg(pText);
        m_pServer->PushToAll(pText);
    }
        break;
    default:
        std::cout << "Unkown Message Type!" << std::endl;
        break;

    }
    return ret;
}

bool SimpleServer::CSession::LoginAuthentication(SimpleServer::CSession::AuthInfo &info)
{
    return !m_pServer->CheckNameExisted(info.UserName);
}


