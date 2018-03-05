#ifndef SIMPLECHAT_H
#define SIMPLECHAT_H

#include <set>
#include <string>
#include <list>

#include <sys/select.h>

#include "simpleMessage.h"


class Guest
{
public:
	Guest(std::string &name, int fd = -1);
	~Guest();

	int PostMessage(simpleMessage *msg);
	simpleMessage *RecvMessage();

    void SetName(std::string &name){m_SelfName = name;}
    void SetFileDescr(int fd) {m_FD = fd;}
    std::string GetName() const {return m_SelfName;}
    int GetFileDescr() const {return m_Fd;}

private:

	int m_Fd;//read or write msg
	std::string m_SelfName;
};


class SimpleChat
{
public:
    SimpleChat(const char *name, int port);
    virtual ~SimpleChat();
    virtual int Init() = 0;
    virtual void Start() = 0;
protected:

    virtual int HandleMsg(simpleMessage *msg, std::list<Guest>::iterator it) = 0;

	std::list<Guest> m_Guests;
    int m_Port;
};


class ChatHost : public SimpleChat
{
public:
    ChatHost(const char *name, int port);

    //overrides
    int Init() override;
    void Start() override;
	
protected:

    int HandleMsg(simpleMessage *msg , std::list<Guest>::iterator it);


private:
    int m_ListenFd;
	fd_set m_FdSet;

};

class ChatGuest : public SimpleChat
{
public:
    ChatGuest();
};


#endif // SIMPLECHAT_H
