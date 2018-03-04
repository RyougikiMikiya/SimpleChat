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
	Guest(std::string &name, int fd = -1);
	~Guest();

	bool checkIf(const Guest &lhs) const
	{
		if (lhs.m_SelfName.empty())
		{
			return m_Fd < lhs.m_Fd;
		}
		else
		{
			return m_SelfName < lhs.m_SelfName;
		}
	}

	int PostMessage(simpleMessage *msg);
	simpleMessage *RecvMessage();
	void setFd(int msgObj) { m_Fd = msgObj; }


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
	std::vector<Guest> m_Guests;
    int m_Port;
};

class ChatHost : public SimpleChat
{
public:
    ChatHost(const char *name, int port);

    //overrides
    int Init() override;
    void Start() override;

private:
    int m_ListenFd;
	fd_set m_FdSet;

};

//class ChatGuest : public SimpleChat
//{
//public:
//    ChatGuest();
//};


#endif // SIMPLECHAT_H
