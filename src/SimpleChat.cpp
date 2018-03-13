
#include <iostream>
#include <algorithm>
#include <sstream>
#include <utility>

#include <cerrno>
#include <cassert>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/signal.h>

#include "SimpleUtility.h"
#include "SimpleChat.h"

SimpleChat::SimpleChat(int port): m_Port(port), m_SelfSession(NULL)
{
}

SimpleChat::~SimpleChat()
{
}

int SimpleChat::RemoveSession(SessionPair *pPair)
{
    SessionIter it = std::find(m_Guests.begin(), m_Guests.end(), pPair);
    if (it == m_Guests.end())
        return -1;
    m_Guests.erase(it);
    return 0;
}

SimpleChat::SessionIter SimpleChat::FindSessionByName(const std::string &name)
{
    SessionIter it = find_if(m_Guests.begin(), m_Guests.end(), [name](SessionPair *s)
    {
        return s->pSession->GetName() == name;
    });
    return it;
}

SimpleChat::SessionIter SimpleChat::FindSessionByFD(int fd)
{
    SessionIter it = find_if(m_Guests.begin(), m_Guests.end(), [fd](SessionPair *s)
    {
        return s->fd == fd;
    });
    return it;
}

ChatHost::ChatHost(int port) : SimpleChat(port), m_hListenFD(-1), m_bStart(false),
    m_hEpollRoot(-1)
{

}

int ChatHost::Init(const char *pName)
{
    int ret;
    std::string name(pName);
    m_SelfSession = new SessionPair;
    if(!m_SelfSession)
        return -1;
    m_SelfSession->pSession = new SessionInHost(STDIN_FILENO);
    if(!m_SelfSession->pSession)
    {
        delete m_SelfSession;
        return -1;
    }
    m_SelfSession->fd = STDIN_FILENO;
    m_SelfSession->pSession->SetName(name);
    m_Guests.push_back(m_SelfSession);

    m_hListenFD = socket(AF_INET, SOCK_STREAM, 0);
    if(m_hListenFD < 0)
        return -1;

    int on = 1;
    ret = setsockopt(m_hListenFD, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(ret < 0)
    {
        std::cerr << strerror(errno) << std::endl;
        close(m_hListenFD);
        return -1;
    }

    struct sockaddr_in servAddr;
    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(m_Port);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(m_hListenFD, (sockaddr*)&servAddr, sizeof(servAddr));
    if(ret < 0)
    {
        std::cerr << strerror(errno) << std::endl;
        close(m_hListenFD);
        return -1;
    }

    ret = listen(m_hListenFD, 20);
    if(ret < 0)
    {
        std::cerr << strerror(errno) << std::endl;
        close(m_hListenFD);
        return -1;
    }

    m_hEpollRoot = epoll_create(1);
    if (m_hEpollRoot < 0)
    {
        std::cerr << "Epoll create err num: " << errno << strerror(errno) << std::endl;
        ret = close(m_hListenFD);
        return -1;
    }

    SessionPair *pListen = new SessionPair;
    if(!pListen)
        return -1;
    pListen->fd = m_hListenFD;
    pListen->pSession = NULL;

    ret = RegistEpoll(m_hListenFD, pListen);
    if(ret < 0)
        std::cerr << "Regist" << m_hListenFD << "epoll err" << strerror(errno) << std::endl;
    ret = RegistEpoll(STDIN_FILENO, m_SelfSession);
    if(ret < 0)
        std::cerr << "Regist " <<  STDIN_FILENO <<"epoll err" << strerror(errno) << std::endl;
    return ret;
}

int ChatHost::Uninit()
{
    assert(!m_bStart);
    for(SessionIter it = m_Guests.begin(); it != m_Guests.end();)
    {
        epoll_ctl(m_hEpollRoot, EPOLL_CTL_DEL, (*it)->fd, NULL);
        delete (*it)->pSession;
        close((*it)->fd);
        delete *it;
        it = m_Guests.erase(it);
    }
    close(m_hEpollRoot);
    return 0;
}


int ChatHost::Start()
{

    int ret = 0;
    m_bStart = true;
    ret = pthread_create(&m_hThread, NULL, EpollThread, this);
    if(ret < 0)
    {
        m_bStart = false;
        return -1;
    }
    return ret;
}

int ChatHost::Stop()
{
    assert(m_bStart && m_hThread > 0);
//    m_bStart = false;
    int ret = pthread_join(m_hThread, NULL);
    return ret;
}

void ChatHost::PushToAll(SimpleMsgHdr *pMsg)
{
    int ret;
    for(SessionIter it = m_Guests.begin(); it != m_Guests.end(); ++it)
    {
        ret = (*it)->pSession->PostMessage(pMsg);
        if(ret < 0)//dosmt
        {
            ;
        }
    }
}

bool ChatHost::LoginAuthentication(AuthenInfo &info)
{
    SessionIter it = FindSessionByName(info.Name);
    return it == m_Guests.end() ? true : false;
}

const char *ChatHost::GetSelfName() const
{
    assert(m_SelfSession);
    return m_SelfSession->pSession->GetName();
}

void *ChatHost::EpollThread(void *param)
{
    assert(param);
    std::cout << "Epoll Thread Start~" << std::endl;

    int ret;
    int nReady;
    ChatHost *pServer = static_cast<ChatHost*>(param);

    epoll_event events[1024];
    SessionPair *pPair;

    while (pServer->m_bStart)
    {
        nReady = epoll_wait(pServer->m_hEpollRoot, events, 1024, -1);
        if (nReady < 0)
        {
            std::cerr << "epoll wait err:" << strerror(errno) << std::endl;
            return NULL;
        }
        for (int i = 0; i < nReady; ++i)
        {
            pPair = static_cast<SessionPair*>(events[i].data.ptr);
            if (pPair->fd == pServer->m_hListenFD && events[i].events == EPOLLIN)
            {
                sockaddr_in cliAddr;
                socklen_t cliLen;
                int cliFD = accept(pServer->m_hListenFD, (sockaddr *)&cliAddr, &cliLen);
                if (cliFD < 0)
                {
                    std::cerr << "Accept err:" << strerror(errno) << std::endl;
                    break;
                }
                pServer->AddSession(cliFD);
            }
            else if (events[i].events == EPOLLIN)
            {
                ret = pPair->pSession->OnRecvMessage(dynamic_cast<SimpleChat*>(pServer));
                if(ret < 0)
                {
                    std::cerr << "Handle session err!" << std::endl;
                    pServer->RemoveSession(pPair);
                }
            }
            else if (events[i].events == EPOLLHUP)
            {
                pServer->RemoveSession(pPair);
            }
        }
    }
    std::cout << "Leave epoll thread ~" << std::endl;

    return NULL;
}

int ChatHost::RegistEpoll(int fd, SessionPair *pPair)
{
    assert(fd >= 0);
    assert(m_hEpollRoot >= 0);
    assert(pPair);
    int ret;
    uint32_t events = EPOLLIN;
    if(fd != m_hListenFD)
        events |= EPOLLHUP;

    epoll_event ev;
    bzero(&ev, sizeof(ev));
    ev.events = events;
    ev.data.ptr = (void *)pPair;
    ret = epoll_ctl(m_hEpollRoot, EPOLL_CTL_ADD, fd, &ev);

    return ret;
}

int ChatHost::UnregistEpoll(int fd)
{
    assert(fd >= 0);
    int ret;
    if(FindSessionByFD(fd) == m_Guests.end())
        return 0;
    ret = epoll_ctl(m_hEpollRoot, EPOLL_CTL_DEL, fd, NULL);
    return ret;
}

int ChatHost::AddSession(int fd)
{
    assert(fd >= 0);
    SimpleSession *pSession = new SessionInHost(fd);
    if(!pSession)
        return -1;
    SessionPair *pPair = new SessionPair;
    if(!pPair)
        return -1;
    pPair->fd = fd;
    pPair->pSession = pSession;
    m_Guests.push_back(pPair);

    assert(m_hEpollRoot >= 0);
    int ret = RegistEpoll(fd, pPair);
    if(ret < 0)
    {
        close(fd);
        delete pSession;
        m_Guests.pop_back();
    }

    return ret;
}

int ChatHost::RemoveSession(SimpleChat::SessionPair *pPair)
{
    assert(pPair);
    SessionPair *tmpPair = pPair;
    int ret;
    ret = UnregistEpoll(pPair->fd);
    if(ret < 0)
        return ret;
    ret = SimpleChat::RemoveSession(pPair);
    close(tmpPair->fd);
    delete tmpPair->pSession;
    delete tmpPair;
    return ret;
}

ChatGuest::ChatGuest(int port) : SimpleChat(port), m_hSocket(-1), m_HostIP(0), m_hostSession(NULL)
{

}

int ChatGuest::Init(const char *pIP, const char *pName)
{
    int ret;
    std::string name(pName);
    m_SelfSession = new SessionPair;
    if(!m_SelfSession)
        return -1;
    m_SelfSession->pSession = new SessionInGuest(STDIN_FILENO);
    if(!m_SelfSession->pSession)
    {
        delete m_SelfSession;
        return -1;
    }
    m_SelfSession->fd = STDIN_FILENO;
    m_SelfSession->pSession->SetName(name);
    m_Guests.push_back(m_SelfSession);


    ret = inet_pton(AF_INET, pIP, &m_HostIP);
    if(ret != 1)
    {
        std::cout << "Invaild IP address!Please enter IPV4 addr!" << std::endl;
        return -1;
    }
    m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_hSocket < 0)
    {
        std::cerr << errno << "  " << strerror(errno) << std::endl;
        return m_hSocket;
    }
    return 0;
}

int ChatGuest::Start()
{
    int ret;
    ret = ConnectHost();
    if(ret < 0)
        return ret;
    assert(m_hostSession);
    ret = Login();
    //客户端目前只能收
    if(ret == HANDLEMSGRESULT_LOGINAUTHSUCCESS)
    {
        while(1)
        {
            ret = m_hostSession->pSession->OnRecvMessage(this);
            if(ret < 0)
                break;
        }
    }
    //remove..
    return 0;
}

int ChatGuest::ConnectHost()
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

int ChatGuest::Login()
{
    SessionIter it = FindSessionByFD(m_hSocket);
    assert(it != m_Guests.end());
    char *pBuf = new char[1024];
    if(!pBuf)
        return -1;
    assert(m_SelfSession->pSession);
    int nameLen = strlen(m_SelfSession->pSession->GetName());
    assert(nameLen > 0);
    LoginMessage *pLogin = new(pBuf) LoginMessage(nameLen);
    memcpy(pLogin->Payload, m_SelfSession->pSession->GetName(), nameLen);
    int ret = (*it)->pSession->PostMessage(pLogin);
    pLogin->~LoginMessage();
    delete [] pBuf;
    ret = (*it)->pSession->OnRecvMessage(this);
    return ret;
}

int ChatGuest::AddSession(int fd)
{
    assert(fd >= 0);
    SessionPair *pPair = new SessionPair;
    if(!pPair)
        return -1;
    SimpleSession *pSession = new SessionInGuest(fd);
    if(!pSession)
        return -1;
    pPair->fd = fd;
    pPair->pSession = pSession;
    m_Guests.push_back(pPair);
    return 0;
}
