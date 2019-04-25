#include <iostream>
#include <utility>

#include <cstring>
#include <cassert>

#include <unistd.h>
#include <sys/epoll.h>

#include "Log/SimpleLog.h"
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

    //init epoll
    m_hEpollRoot = epoll_create(1);
    if (m_hEpollRoot < 0)
    {
        DLOGERROR(strerror(errno));
        return -1;
    }

    //init unamed sem
    ret = sem_init(&m_sem, 0, 0);
    if (ret < 0)
    {
        DLOGERROR(strerror(errno));
        ret = close(m_hEpollRoot);
        if (ret < 0)
        {
            DLOGERROR(strerror(errno));
        }
        m_hEpollRoot = -1;
    }

    return ret;
}

int SimpleListener::UninitListener()
{
    assert(this);
    assert(m_hEpollRoot >= 0);
    assert(!m_bStart);
    assert(m_Receviers.empty());

    int ret = close(m_hEpollRoot);
    if (ret < 0)
    {
        DLOGERROR(strerror(errno));
    }
    m_hEpollRoot = -1;

    ret = sem_destroy(&m_sem);
    if (ret < 0)
    {
        DLOGERROR(strerror(errno));
    }

    return ret;
}

int SimpleListener::Start()
{
    assert(this);
    int ret = 0;
    assert(m_bStart == false);
    assert(m_hEpollRoot >= 0);
    m_bStart = true;
    ret = pthread_create(&m_hThread, NULL, EpollThread, this);
    if ( ret < 0 )
    {
        DLOGERROR(strerror(errno));
        m_bStart = false;
    }
        
    return ret;
}

int SimpleListener::Stop()
{
    assert(this);
    int ret = 0;
    if (!m_bStart)
    {
        DLOGWARN("Listener has been stopped!");
        return 0;
    }
    m_bStart = false;
    assert(m_hEpollRoot >= 0);
    ret = pthread_join(m_hThread, NULL);
    if (ret < 0)
    {
        DLOGERROR(strerror(errno));
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
    uint32_t events = EPOLLIN;
    struct epoll_event ev;

    bzero(&ev, sizeof(ev));
    ev.events = events;
    ev.data.ptr = pReceiver;
    ret = epoll_ctl(m_hEpollRoot, EPOLL_CTL_ADD, fd, &ev);
    DLOGDEBUG("Add fd %d to epoll", fd);
    if (ret < 0)
    {
        DLOGERROR(strerror(errno));
        return ret;
    }
    auto pair = m_Receviers.insert({fd, pReceiver});
    assert(pair.second);

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
    ret = epoll_ctl(m_hEpollRoot, EPOLL_CTL_DEL, fd, NULL);
    DLOGDEBUG("Del fd %d from epoll", fd);
    if (ret < 0)
        DLOGERROR(strerror(errno));
    return ret;
}

void *SimpleListener::EpollThread(void *param)
{
    assert(param);
    DLOGINFO("Epoll thread start~");
    int nReady;
    SimpleListener *pListener = static_cast<SimpleListener *>(param);

    epoll_event events[1024];
    IReceiver *pReceiver;

    while (pListener->m_bStart)
    {
        //LT模式下，如果socket出错（对端关闭），如果没有从epoll上移除，也没关闭的话。会不停的触发可读事件。
        nReady = epoll_wait(pListener->m_hEpollRoot, events, 1024, 500);
        if( nReady == 0)
            continue;
        if ( nReady < 0 )
        {
            if( errno == EINTR );
                continue;
            DLOGERROR(strerror(errno));
            return NULL;
        }
        DLOGDEBUG("Epoll return %d", nReady);
        for (int i = 0; i < nReady; ++i)
        {
            pReceiver = reinterpret_cast<IReceiver *>(events[i].data.ptr);
            assert(pReceiver);
            if (events[i].events & EPOLLIN)
            {
                DLOGDEBUG("Epoll recv EPOLLIN");
            }
            else if (events[i].events & EPOLLRDHUP)
            {
                DLOGDEBUG("Epoll recv EPOLLRDHUP");
            }
            DLOGDEBUG("%p call OnReceive", pReceiver);
            pReceiver->OnReceive();
            DLOGDEBUG("%p call finish OnReceive", pReceiver);
        }
    }
    DLOGINFO("Leave thread start~");

    return NULL;
}
