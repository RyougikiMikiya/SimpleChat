#ifndef SIMPLECHAT_H
#define SIMPLECHAT_H

#include <string>
#include <vector>
#include <map>

#include <sys/select.h>
#include <netinet/in.h>
#include <pthread.h>

#include <sys/epoll.h>

#include "SimpleSession.h"
#include "SimpleMessage.h"


class SimpleSession;
class SessionInHost;
class SessionInGuest;

/*
 * SimpleChat manage lots of GuestSession handle.It doesn't directly modify a session inner attr.
*/

class SimpleChat
{
public:
    SimpleChat(int port);
    virtual ~SimpleChat();

protected:
    struct SessionPair
    {
        int fd;
        SimpleSession *pSession;
    };

    typedef std::vector<SessionPair*> SessionList;
    typedef SessionList::iterator SessionIter;
    typedef SessionList::const_iterator SessionConstIter;

    virtual int AddSession(int fd) = 0;

    virtual int RemoveSession(SessionPair *session);

    SessionIter FindSessionByName(const std::string &name);
    SessionIter FindSessionByFD(int fd);

    SessionList m_Guests;
    int m_Port;
    SessionPair *m_SelfSession;
};


struct AuthenInfo
{
    AuthenInfo(const std::string &name) : Name(name){}
    std::string Name;
};



class ChatHost : public SimpleChat
{
public:
    ChatHost(int port);
    ~ChatHost(){}

    int Init(const char *pName);
    int Uninit();

    int Start();
    int Stop();

    //fuction for session
public:
    void PushToAll(SimpleMsgHdr *pMsg);
    bool LoginAuthentication(AuthenInfo &info);
    const char *GetSelfName() const;

private:
    static void *EpollThread(void *param);

    int CreateEpoll();
    int RegistEpoll(int fd, SessionPair *pPair);
    int UnregistEpoll(int fd);

    //overrides
    int AddSession(int fd);
    int RemoveSession(SessionPair *pPair);

    int m_hListenFD;

    bool m_bStart;
    int m_hEpollRoot;
    pthread_t m_hThread;
};


class ChatGuest : public SimpleChat
{
public:
    ChatGuest(int port);

    int Init(const char *pIP, const char *pName) ;
    int Start() ;

private:
    int ConnectHost();
    int Login();

    //overrides
    int AddSession(int fd);

private:
    int m_hSocket;
    in_addr_t m_HostIP;
    SessionPair *m_hostSession;
};


#endif // SIMPLECHAT_H
