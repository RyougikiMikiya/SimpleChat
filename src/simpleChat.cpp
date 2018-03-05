#include <iostream>
#include <algorithm>
#include <sstream>

#include <cerrno>
#include <cassert>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>

#include <sys/signal.h>

#include "simpleChat.h"

Guest::Guest(std::string &name, int fd) : m_SelfName(name), m_Fd(fd)
{
}

Guest::~Guest()
{
	if (m_Fd > 0)
		close(m_Fd);
}

int Guest::PostMessage(simpleMessage * msg)
{

	return 0;
}

simpleMessage *Guest::RecvMessage()
{
	return nullptr;
}

/*

*/

SimpleChat::SimpleChat(std::string &name, int port): m_Port(port)
{
    m_Guests.push_back(Guest(name, STDIN_FILENO));
}

SimpleChat::~SimpleChat()
{
}

ChatHost::ChatHost(std::string &name, int port) : SimpleChat(name, port)
{

}

int ChatHost::Init()
{
	int ret;
	if ((m_ListenFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		std::cerr << errno << "  " << strerror(errno) << std::endl;
		return m_ListenFd;
	}
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(m_Port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int opt = 1;
	if ((ret = setsockopt(m_ListenFd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt))) < 0)
	{
		std::cerr << errno << "  " << strerror(errno) << std::endl;
		return ret;
	}
	if ((ret = bind(m_ListenFd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0))
	{
		std::cerr << errno << "  " << strerror(errno) << std::endl;
		return ret;
	}
	if ((ret = listen(m_ListenFd, 20)) < 0)
	{
		std::cerr << errno << "  " << strerror(errno) << std::endl;
		return ret;
	}
	return 0;
}

void ChatHost::Start()
{
	char buf[BUFSIZ];

	int nReady, maxFd, tempFd;

	FD_ZERO(&m_FdSet);
	FD_SET(m_ListenFd, &m_FdSet);
	FD_SET(STDIN_FILENO, &m_FdSet);
	maxFd = m_ListenFd;

	while (1)
	{
		fd_set rfd = m_FdSet;//监听的rfd 因为下面可能添加新的clifd到m_fdSet里。
		nReady = select(maxFd + 1, &rfd, NULL, NULL, NULL);
		if (nReady < 0)
		{
			std::cerr << nReady << "  " << errno << "  " << strerror(errno) << std::endl;
		}

		if (FD_ISSET(m_ListenFd, &rfd))
		{
            sockaddr_in cliaddr;
            socklen_t cliLen = sizeof(cliaddr);
            int cliFd = accept(m_ListenFd, (sockaddr*)&cliaddr, &cliLen);
			if (cliFd < 0)
			{
				std::cerr << cliFd << "  " << errno << "  " << strerror(errno) << std::endl;
				continue;
			}
			std::cout << inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, buf, sizeof buf) << std::endl;
			FD_SET(cliFd, &m_FdSet);
            std::string anonym;
            m_Guests.push_back(Guest(anonym, cliFd));

			if (cliFd > maxFd)
				maxFd = cliFd;
            --nReady;
            if (nReady == 0)//没有更多的事件了
				continue;
		}
		
		for(auto it = m_Guests.begin(); it != m_Guests.end(); it++)
		{
            tempFd = it->GetFileDescr();
			if(FD_ISSET(tempFd, &rfd))
			{
				simpleMessage *msg = it->RecvMessage();
				assert(msg);
                HandleMsg(msg, it);
                --nReady;
                if(nReady == 0)
					break;
			}
		}
	}
}

int ChatHost::HandleMsg(simpleMessage *msg, std::list<Guest>::iterator it)
{
    switch(msg->GetMsgID())
	{
		case SPLMSG_LOGIN:
		{
            std::string name(msg->GetPayload());
            auto position = std::find_if(m_Guests.begin(), m_Guests.end(), [name](Guest & guest){
                    return guest.GetName() == name;
			});
            if(position != m_Guests.end())
            {
                simpleMessage *errMsg = new simpleMessage(SPLMSG_ERR);
                std::stringstream contents;
                contents << "Name :'" << name << "' has existed!";
                errMsg->AddPayload(contents);
                it->PostMessage(errMsg);
                m_Guests.erase(it);
            }
            else
            {
                it->SetName(name);
                simpleMessage *loginOkMsg = new simpleMessage(SPLMSG_OK);
                it->PostMessage(loginOkMsg);
            }
		}
		break;
		case SPLMSG_TEXT:
		{
            for(auto it = m_Guests.begin(); it != m_Guests.end(); ++it)
            {
                it->PostMessage(msg);
            }
		}
		break;
	}
    return 0;
}


