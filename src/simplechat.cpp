#include <iostream>
#include <algorithm>

#include <cerrno>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>

#include <sys/signal.h>

#include "simplechat.h"

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

simpleMessage * Guest::RecvMessage()
{
	return nullptr;
}





SimpleChat::SimpleChat(const char *name, int port): m_Port(port)
{
	m_Guests.emplace_back(name);
}

SimpleChat::~SimpleChat()
{
}

ChatHost::ChatHost(const char *name, int port) : SimpleChat(name, port)
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

	sockaddr_in cliaddr;
	socklen_t cliLen = sizeof(cliaddr);
	int nReady, maxFd;

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
			int cliFD = accept(m_ListenFd, (sockaddr*)&cliaddr, &cliLen);
			if (cliFD < 0)
			{
				std::cerr << cliFD << "  " << errno << "  " << strerror(errno) << std::endl;
				continue;
			}
			std::cout << inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, buf, sizeof buf) << std::endl;
			FD_SET(cliFD, &m_FdSet);
			auto it = std::find_if(m_Guests.begin(), m_Guests.end(), []() {return true; });



			if (cliFD > maxFd)
				maxFd = cliFD;
			if (--nReady == 0)//没有更多的事件了
				continue;
		}
		
		
	}
}

