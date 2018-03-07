#ifndef SIMPLECHAT_H
#define SIMPLECHAT_H

#include <set>
#include <string>
#include <vector>

#include <sys/select.h>

#include "simpleMessage.h"





/*
    Now not handle the err such as EINTER ,just print to STDIN and return -1.

*/


/*
    When the GuestSession is created, it means that the file descriptor should be valid.
    Session Provide lots of function to set/get attr.
*/

class GuestSession
{
public:
    GuestSession(int fd);
    ~GuestSession();

    int PostMessage(SimpleMessage *msg);
    SimpleMessage *RecvMessage();

    //function for attr
    void SetName(const std::string &name) {m_Attr.GuestName = name;}
    std::string GetName() const {return m_Attr.GuestName;}

    int GetFileDescr() const {return m_FD;}

private:
    //Attributes for each session guest
    struct GuestAttr
    {
        std::string GuestName;
        //some ohter attrs
    };

    int m_FD;//read or write msg
    long m_SessionID;//only id for each
    GuestAttr m_Attr;
};


/*
 * SimpleChat manage lots of GuestSession handle.It doesn't directly modify a session inner attr.
*/

class SimpleChat
{
public:
    SimpleChat(int port);
    virtual ~SimpleChat();
    virtual int Create(int argc, char **argv) = 0;
    virtual int Init() = 0;
    virtual int Run() = 0;
protected:
    typedef std::vector<GuestSession*>::iterator GuestsIter;
    typedef std::vector<GuestSession*>::const_iterator GuestsConstIter;

    int AddSession(int fd);
    int CloseSession(GuestSession *session);

    GuestsIter FindSessionByFD(int fd);
    GuestsIter FindSessionByName(const std::string &name);

    virtual int HandleMsg(SimpleMessage *pMsg, GuestSession *pSender) = 0;

    std::vector<GuestSession*> m_Guests;

    int m_Port;
};


class ChatHost : public SimpleChat
{
public:
    ChatHost(int port);

    //overrides
    int Create(int argc, char **argv);
    int Init() override;
    int Run() override;
    
protected:

    int HandleMsg(SimpleMessage *pMsg, GuestSession *pSender);


private:
    int m_ListenFd;
    fd_set m_FdSet;

};

class ChatGuest : public SimpleChat
{
public:
    ChatGuest(int port);
};


#endif // SIMPLECHAT_H
