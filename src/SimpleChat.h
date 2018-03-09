#ifndef SIMPLECHAT_H
#define SIMPLECHAT_H

#include <string>
#include <vector>
#include <map>

#include <sys/select.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/epoll.h>

#include "SimpleMessage.h"





/*
    Now not handle the err such as EINTER ,just print to STDIN and return -1.

*/


/*
    When the GuestSession is created, it means that the file descriptor should be valid.
    Session Provide lots of function to set/get attr.
*/



//Attributes for each session guest
struct GuestAttr
{
    std::string GuestName;
    //some ohter attrs
};

class IListener
{
public:
    virtual ~IListener();
    virtual int OnReceive() = 0;
};

class GuestSession : public IListener
{
public:
    GuestSession(int fd);
    ~GuestSession();

    //overrides
    int OnReceive();

public:
    //function for setting attr
    void SetName(const std::string &name) {m_Attr.GuestName = name;}

    const char *GetName() const {return m_Attr.GuestName.c_str();}
    int GetSessionID() const {return m_SessionID;}

private:
    int PostMessage(SimpleMsgHdr *pMsg);
    SimpleMsgHdr *RecvMessage();
    int HandleMessage(SimpleMsgHdr *pMsg);

private:

    long m_SessionID;//only id for each
    GuestAttr m_Attr;
    int m_FD;
};

/*
 * SimpleChat manage lots of GuestSession handle.It doesn't directly modify a session inner attr.
*/

class SimpleChat
{
public:
    SimpleChat(int port);
    virtual ~SimpleChat();
protected:
    typedef std::vector<GuestSession*>::iterator GuestsIter;
    typedef std::vector<GuestSession*>::const_iterator GuestsConstIter;

    int AddSession(int fd);
    int CloseSession(GuestSession *session);

    GuestSession *FindSessionByFD(int fd);
    GuestsIter FindSessionByName(const std::string &name);
    GuestsIter FindSessionByID(long sid);

    virtual int HandleMsg(SimpleMsgHdr *pMsg, GuestSession *pSender) = 0;

    std::vector<GuestSession*> m_Guests;

    GuestSession m_SelfSession;
    int m_Port;
};


class Selector
{

public:
    Selector();
    ~Selector();

    int RegisterListener(int fd, IListener* pListener);
    int UnRegisterListener(int fd, IListener *pListener);

    int Init();
    int Uninit();
    int Start();
    int Stop();

private:
    static void *StartThread(void *pParam);
    int m_hRoot;
    bool m_bStart;
    pthread_t m_pThread;
};

class ChatHost : public SimpleChat, public IListener
{
public:
    ChatHost(int port);
    ~ChatHost();
    //overrides
    int OnReceive();

    int Init(const char *pName);
    int Uninit();

    int Start();
    int Stop();

private:
    int m_hListenFD;
    Selector m_Select;
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
