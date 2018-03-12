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

/*
 * SimpleChat manage lots of GuestSession handle.It doesn't directly modify a session inner attr.
*/

class SimpleChat
{
public:
    SimpleChat(int port);
    virtual ~SimpleChat();

    const char * GetSelfName() const {return m_SelfSession.GetName();}

protected:
    struct SessionPair
    {
        int fd;
        SimpleSession *pSession;
    };


    typedef std::vector<SessionPair>::iterator GuestsIter;
    typedef std::vector<SessionPair>::const_iterator GuestsConstIter;

    virtual int AddSession(int fd) = 0;

    virtual int RemoveSession(SessionPair *session);

    GuestsIter FindSessionByName(const std::string &name);
    GuestsIter FindSessionByFD(int fd);

    std::vector<SessionPair> m_Guests;

    SimpleSession m_SelfSession;
    int m_Port;
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


private:
    static void *EpollThread(void *param);

    int CreateEpoll();
    int RegistEpoll(int fd, SessionPair *pPair);
    int UnregistEpoll(int fd);

    //overrides
    int AddSession(fd);
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
    int Run() ;

private:
    int ConnectHost();
    int Login();


private:
    int m_hSocket;
    in_addr_t m_HostIP;
};


#endif // SIMPLECHAT_H
