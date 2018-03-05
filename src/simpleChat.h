#ifndef SIMPLECHAT_H
#define SIMPLECHAT_H

#include <set>
#include <string>
#include <vector>

#include <sys/select.h>

#include "simpleMessage.h"


class Guest
{
public:
    Guest(const std::string &name, int fd = -1);
	~Guest();

	int PostMessage(simpleMessage *msg);
	simpleMessage *RecvMessage();

    void SetName(const std::string &name){m_SelfName = name;}
    void SetFileDescr(int fd) {m_Fd = fd;}
    std::string GetName() const {return m_SelfName;}
    int GetFileDescr() const {return m_Fd;}

private:

	int m_Fd;//read or write msg
	std::string m_SelfName;
};


class SimpleChat
{
public:
    SimpleChat(std::string &name, int port);
    virtual ~SimpleChat();
    virtual int Init() = 0;
    virtual void Start() = 0;
protected:

    virtual int HandleMsg(simpleMessage *msg, std::list<Guest>::iterator it) = 0;

    std::vector<Guest*> m_Guests;
    int m_Port;
};


class ChatHost : public SimpleChat
{
public:
    ChatHost(std::string &name, int port);

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
