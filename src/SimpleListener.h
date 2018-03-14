#ifndef SIMPLELISTENER_H
#define SIMPLELISTENER_H

#include <map>

#include <pthread.h>

class IReceiver
{
public:
    virtual ~IReceiver(){}
    virtual void OnReceive() = 0;
};


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
    bool m_bStart;
    pthread_t m_hThread;
    RecevierList m_Receviers;


};

#endif // SIMPLELISTENER_H
