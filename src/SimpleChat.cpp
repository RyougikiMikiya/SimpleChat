
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

SimpleChat::SimpleChat(int port): m_Port(port) , m_SelfSession(STDIN_FILENO), m_Select()
{

}

SimpleChat::~SimpleChat()
{
}

int SimpleChat::RemoveSession(SessionPair *pPair)
{
    GuestsIter it = std::find(m_Guests.begin(), m_Guests.end(), *pPair);
    if (it == m_Guests.end())
        return -1;
    m_Guests.erase(it);
    return 0;
}

SimpleChat::GuestsIter SimpleChat::FindSessionByName(const std::string &name)
{
    GuestsIter it = find_if(m_Guests.begin(), m_Guests.end(), [name](SessionPair &s)
    {
        return s.pSession->GetName() == name;
    });
    return it;
}

SimpleChat::GuestsIter SimpleChat::FindSessionByFD(int fd)
{
    GuestsIter it = find_if(m_Guests.begin(), m_Guests.end(), [fd](SessionPair &s)
    {
        return s.fd == fd;
    });
    return it;
}

ChatHost::ChatHost(int port) : m_hListenFD(-1), SimpleChat(port)
{

}

int ChatHost::Init(const char *pName)
{
    int ret;
    std::string name(pName);
    m_SelfSession.SetName(name);



    m_hListenFD = socket(AF_INET, SOCK_STREAM, 0);
    if(m_hListenFD < 0)
        return -1;

    int on = 1;
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(ret < 0)
    {
        std::cerr << strerror(errno) << std::endl;
        close(m_hListenFD);
        return -1;
    }

    struct sockaddr_in servAddr;
    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr = htons(m_Port);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(m_hListenFD, (sockaddr*)servAddr, sizeof(servAddr));
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

    RegistEpoll(m_hListenFD, NULL);

    return 0;
}


int ChatHost::Start()
{
    int ret;
    ret = pthread_create(m_hThread, NULL, EpollThread, this);
    pthread_join(m_hThread, NULL);
    return ret;
}

void ChatHost::PushToAll(SimpleMsgHdr *pMsg)
{
    for(GuestsIter it = m_Guests.begin(); it != m_Guests.end(); ++it)
    {
        ret = it->pSession->PostMessage(pMsg);
        if(ret < 0)//dosmt
        {
            ;
        }
    }
}

bool ChatHost::LoginAuthentication(AuthenInfo &info)
{
    GuestsIter it = FindSessionByName(info.Name);
    return it == m_Guests.end() ? true : false;
}

void *ChatHost::EpollThread(void *param)
{
    int nReady;
    ChatHost *pServer = static_cast<ChatHost*>(param);

    epoll_event events[1024];
    SessionPair *pSession;

    while (pServer->m_bStart)
    {
        nReady = epoll_wait(pServer->m_hEpollRoot, events, 1024, -1);
        if (nReady < 0)
        {
            return NULL;
        }
        for (int i = 0; i < nReady; ++i)
        {
            pSession = static_cast<SessionPair*>(events[i].data.ptr);
            if (pSession->fd == pServer->m_hListenFD && events[i].events == EPOLLIN)
            {
                sockaddr_in cliAddr;
                socklen_t cliLen;
                int cliFD = accept(pServer->m_hListenFD, (sockaddr *)&cliAddr, &cliLen);
                if (cliFD < 0)
                {
                    //
                }
                pServer->AddSession(cliFD);
            }
            else if (events[i].events == EPOLLIN)
            {
                pSession->pSession->OnRecvMessage(dynamic_cast<SimpleChat*>pServer);
            }
            else if (events[i].events == EPOLLHUP)
            {
                pServer->RemoveSession(pSession);
            }
        }
    }

    return NULL;
}

int ChatHost::RegistEpoll(int fd, SessionPair *pPair)
{
    assert(fd >= 0);
    int ret;
    EPOLL_EVENTS events = EPOLLIN;
    if(fd != m_hListenFD)
        events |= EPOLLHUP;

    epoll_event ev;
    bzero(&ev, sizeof(ev));
    ev.events = events;
    if(pPair)
    {
        ev.data.ptr = (void *)pPair;
    }
    else
    {
        assert(fd == m_hListenFD);
        ev.data.fd = fd;
    }
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

int ChatHost::AddSession(fd)
{
    assert(fd >= 0);
    SimpleSession *pSession = new SessionInHost(fd);
    if(!pSession)
        return -1;
    SessionPair pair{fd, pSession};
    m_Guests.push_back(pair);

    int ret = RegistEpoll(fd, &m_Guests[m_Guests.size()-1]);
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
    SessionPair tmpPair = *pPair;
    int ret;
    ret = UnregistEpoll(pPair->fd);
    if(ret < 0)
        return ret;
    ret = SimpleChat::RemoveSession(pPair);
    close(tmpPair.fd);
    delete tmpPair.pSession;
    return ret;
}

ChatGuest::ChatGuest(int port) : m_HostIP(0), m_hSocket(-1), SimpleChat(port)
{

}

int ChatGuest::Init(const char *pIP, const char *pName)
{
    int ret;
    std::string name(pName);
    m_SelfSession.SetName(name);
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

int ChatGuest::Run()
{
    int ret;
    ret = ConnectHost();
    if(ret < 0)
        return ret;
    ret = Login();
    /*
     * ......
    */

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

    assert(m_hSocket != -1);

    ret = connect(m_hSocket, (sockaddr *)&servaddr, sizeof(servaddr));
    if(ret < 0)
    {
        std::cerr << errno << "  " << strerror(errno) << std::endl;
        return ret;
    }
    ret = AddSession(m_hSocket);
}

int ChatGuest::Login()
{
    GuestsIter pSelf = FindSessionByFD(STDIN_FILENO);
    GuestsIter pHost = FindSessionByFD(m_hSocket);
    assert(pSelf!=m_Guests.end() && pHost != m_Guests.end());
    NormalMessage *pMsg = (NormalMessage *)malloc(sizeof(SimpleMsgHdr) + (*pSelf)->GetName().size());
    if(!pMsg)
        return -1;
    pMsg->FrameHead = MSG_FRAME_HEADER;
    pMsg->ID = SPLMSG_LOGIN;
    pMsg->Lenth = (*pSelf)->GetName().size();
    memcpy(pMsg->Payload, (*pSelf)->GetName().c_str(), pMsg->Lenth);
    std::cout << "Login payload: " << pMsg->Payload << std::endl;
    (*pHost)->PostMessage(pMsg);
    SimpleMsgHdr *pOkMsg = (*pSelf)->RecvMessage();
    HandleMsg(pOkMsg, *pHost);

}
