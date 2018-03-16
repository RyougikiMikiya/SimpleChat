#include <iostream>
#include <utility>

#include <cstring>
#include <cassert>

#include <unistd.h>
#include <sys/epoll.h>


#include "SimpleListener.h"

SimpleListener::SimpleListener() : m_hEpollRoot(-1), m_bStart(false)
{

}

SimpleListener::~SimpleListener()
{
    assert(m_hEpollRoot == -1);
    assert(m_bStart == false);
}

int SimpleListener::InitListener()
{
    assert(this);
    assert(m_hEpollRoot == -1);
    assert(!m_bStart);
    int ret = 0;
    m_hEpollRoot = epoll_create(1);
    if (m_hEpollRoot < 0)
    {
        std::cerr << "Epoll create ERR num: " << errno << " " << strerror(errno) << std::endl;
        ret = -1;
    }
    return ret;
}

int SimpleListener::UninitListener()
{
    assert(this);
    assert(m_hEpollRoot >=0);
    assert(!m_bStart);
    assert(m_Receviers.empty());
    int ret = close(m_hEpollRoot);
    if(ret < 0)
    {
        std::cerr << "Close Epoll ERR : " << errno << ". " <<strerror(errno) << std::endl;
        ret = -1;
    }
    m_hEpollRoot = -1;

    return ret;
}

int SimpleListener::Start()
{
    assert(this);
    int ret = 0;
    assert(m_bStart == false);
    assert(m_hEpollRoot >= 0);
    if(m_Receviers.empty())
    {
        std::cout << "Listener has no Recevier, please regist first!" << std::endl;
        ret = -1;
        goto ERR;
    }
    m_bStart = true;
    ret = pthread_create(&m_hThread, NULL, EpollThread, this);
    if(ret < 0)
    {
        std::cerr << "Create epoll thread ERR: " << strerror(errno) << std::endl;
        ret = -1;
        goto ERR;
    }
    return 0;

    ERR:

    std::cout << "Start listener failed" << std::endl;
    m_bStart = false;
    return ret;
}

int SimpleListener::Stop()
{
    assert(this);
    int ret = 0;
    if(!m_bStart)
    {
        std::cout << "Listener has been stopped" << std::endl;
        return -1;
    }
    assert(m_hEpollRoot >= 0);
    ret = pthread_join(m_hThread, NULL);
    if(ret < 0)
    {
        std::cerr << "Epoll thread join ERR: " << strerror(errno) << std::endl;
        return -1;
    }
    return ret;
}

int SimpleListener::RegisterRecevier(int fd, IReceiver *pReceiver)
{
    assert(this);
    assert(fd >= 0);
    assert(m_hEpollRoot >= 0);
    assert(pReceiver);
    int ret = 0;
    uint32_t events = EPOLLIN | EPOLLHUP;
    epoll_event ev;

    std::pair<ListIt, bool> pair = m_Receviers.insert({fd, pReceiver});
    if(!pair.second)
        goto ERR;
    bzero(&ev, sizeof(ev));
    ev.events = events;
    ev.data.ptr = pReceiver;
    ret = epoll_ctl(m_hEpollRoot, EPOLL_CTL_ADD, fd, &ev);
    if(ret < 0)
        goto ERR;
    return ret;

    ERR:
    if(pair.second)
    {
        m_Receviers.erase(pair.first);
        std::cerr << "Failed to regist listener : " << strerror(errno) << std::endl;
    }
    std::cerr << "This fd has existed!" << std::endl;
    return ret;
}

int SimpleListener::UnRegisterRecevier(int fd, IReceiver *pReceiver)
{
    assert(this);
    assert(fd >= 0);
    assert(m_hEpollRoot >= 0);
    assert(pReceiver);
    int ret = 0;
    ListIt it = m_Receviers.find(fd);
    assert(it != m_Receviers.end());
    assert(it->second == pReceiver);
    m_Receviers.erase(it);
    ret = epoll_ctl(m_hEpollRoot, EPOLL_CTL_DEL, fd , NULL);
    if(ret < 0)
        std::cerr << "Epoll Unregist "<< fd <<" Err" << strerror(errno) << std::endl;
    return ret;
}

void *SimpleListener::EpollThread(void *param)
{
    assert(param);
    std::cout << "Epoll thread start~" << std::endl;
    int nReady;
    SimpleListener *pListener = static_cast<SimpleListener*>(param);

    epoll_event events[1024];
    IReceiver *pReceiver;

    while (pListener->m_bStart)
    {
        nReady = epoll_wait(pListener->m_hEpollRoot, events, 1024, -1);
        if (nReady < 0)
        {
            std::cerr << "epoll wait ERR:" << strerror(errno) << std::endl;
            return NULL;
        }
        for (int i = 0; i < nReady; ++i)
        {
            pReceiver = reinterpret_cast<IReceiver*>(events[i].data.ptr);
            assert(pReceiver);
            if(events[i].events == EPOLLIN)
                std::cout << "Epoll in event!" << std::endl;
            else if(events[i].events == EPOLLHUP)
                std::cout << "Epoll Hup event!" << std::endl;
            pReceiver->OnReceive();
        }
    }
    std::cout << "Leave epoll thread ~" << std::endl;

    return NULL;
}
