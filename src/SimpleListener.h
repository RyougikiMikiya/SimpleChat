#ifndef SIMPLELISTENER_H
#define SIMPLELISTENER_H

#include <map>

#include <pthread.h>
#include <semaphore.h>

class IReceiver
{
public:
    virtual void OnReceive() = 0;
};


/*
后期：
1.需要实现一个消费者队列来处理收到的epoll请求
2.epoll设计高速模式
*/


class SimpleListener
{
public:
    SimpleListener();
    ~SimpleListener();

public:
    int InitListener();
    int UninitListener();

    int Start();
    int Stop();

    int RegisterRecevier(int fd, IReceiver *pReceiver);
    int UnRegisterRecevier(int fd, IReceiver *pReceiver);

private:
    static void *EpollThread(void *param);

    typedef std::map<int, IReceiver*> RecevierList;
    typedef RecevierList::iterator ListIt;
    typedef RecevierList::const_iterator ListConstIt;
    typedef RecevierList::value_type ListValue;

    int m_hEpollRoot;
    volatile bool m_bStart;
    RecevierList m_Receviers;

    pthread_t m_hThread;
    sem_t m_sem;

};

#endif // SIMPLELISTENER_H
