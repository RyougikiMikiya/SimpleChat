#ifndef SIMPLESESSION_H
#define SIMPLESESSION_H


#include "SimpleChat.h"
#include "SimpleMessage.h"

//Attributes for each session guest
struct GuestAttr
{
    std::string GuestName;
    //some ohter attrs
};

class SimpleSession
{
public:
    SimpleSession(int fd);
    virtual ~SimpleSession();

public:
    virtual int OnRecvMessage(SimpleChat *pHandler) = 0;

    int PostMessage(SimpleMsgHdr *pMsg);

    //function for setting attr
    void SetName(const std::string &name) {m_Attr.GuestName = name;}
    const char *GetName() const {return m_Attr.GuestName.c_str();}
    int GetSessionID() const {return m_SessionID;}

private:
    SimpleMsgHdr *RecvMessage();

private:
    long m_SessionID;//only id for each
    GuestAttr m_Attr;
    int m_FD;

    char m_RecvMsgBuf[2048];
    char m_SendMsgBuf[2048];
};

class SessionInHost : public SimpleSession
{
public:
    //overrides
    int OnRecvMessage(SimpleChat *pHandler);
private:
    int HandleMsgByHost(ChatHost *pHandler, SimpleMsgHdr *pMsg);
};

class SessionInGuest : public SimpleSession
{
public:
    int OnRecvMessage(SimpleChat *pHandler);
private:
    int HandleMsgByGuest(ChatGuest *pHandler, SimpleMsgHdr *pMsg);
};


#endif // SIMPLESESSION_H
