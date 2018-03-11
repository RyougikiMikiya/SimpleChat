﻿
#include <iostream>
#include <algorithm>
#include <sstream>
#include <utility>

#include <cerrno>
#include <cassert>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/signal.h>

#include "SimpleUtility.h"
#include "SimpleChat.h"

static long SimpleID = 0;

#define HANDLEMSGRESULT_DELSESSION        100


GuestSession::GuestSession(int fd) : m_FD(fd), m_SessionID(-1)
{

}

GuestSession::~GuestSession()
{
    assert(m_FD < 0);
}

int GuestSession::PostMessage(SimpleMsgHdr *pMsg)
{
    int result;
    if(m_FD == STDIN_FILENO)
    {
        NormalMessage *pFullMsg = static_cast<NormalMessage*>(pMsg);
        std::cout << pFullMsg->Payload << std::endl;
        return 0;
    }
    else
    {
        result = writen(m_FD, (char*)pMsg, sizeof(SimpleMsgHdr) + pMsg->Lenth);
        if(result < 0)
        {
            std::cerr << "Write err: " << errno << "  " << strerror(errno) << std::endl;
            return -1;
        }
    }
    return 0;
}

SimpleMsgHdr *GuestSession::RecvMessage()
{
    int hdrLenth = sizeof(SimpleMsgHdr);
    char hdrBuf[sizeof(SimpleMsgHdr)];
    int remain, result;

    if(m_FD == STDIN_FILENO)
    {
        std::string line;
        std::getline(std::cin, line);
        NormalMessage *pMsg = (NormalMessage *)malloc(hdrLenth + line.size());
        pMsg->FrameHead = MSG_FRAME_HEADER;
        pMsg->ID = SPLMSG_TEXT;
        pMsg->Lenth = line.size();
        memcpy(pMsg->Payload, line.c_str(), line.size());
        return pMsg;
    }

    else
    {
        bzero(hdrBuf, hdrLenth);
        remain = hdrLenth;
        result = readn(m_FD, hdrBuf, remain);
        if(result < 0)
        {
            std::cerr << "Recv fail: " << errno << "  " << strerror(errno) << std::endl;
            return NULL;
        }
        else if(result != remain)
        {
            std::cerr << "Want to read " << remain << " bytes and actually read " << result <<" bytes " << std::endl;
            return NULL;
        }

        SimpleMsgHdr *pHeader= reinterpret_cast<SimpleMsgHdr*>(hdrBuf);
        if(pHeader->FrameHead != MSG_FRAME_HEADER)
        {
            std::cout << "Message frame head is incorrect!" << std::endl;
            return NULL;
        }
        remain = pHeader->Lenth;
        std::cout << "Remain msg lenth: " << remain << std::endl;

        switch(pHeader->ID)
        {
            //same handle
            case SPLMSG_TEXT:
            case SPLMSG_LOGIN:
            case SPLMSG_LOGIN_OK:
            {
                NormalMessage *pFullMsg = (NormalMessage *)malloc(hdrLenth + remain + 1);
                if(!pFullMsg)
                    return NULL;
                memcpy(pFullMsg, hdrBuf, hdrLenth);
                char *pPayload = pFullMsg->Payload;

                result = readn(m_FD, pPayload, remain);
                if(result < 0)
                {
                    std::cerr << errno << "  " << strerror(errno) << std::endl;
                    return NULL;
                }
                else if(result != remain)
                {
                    std::cerr << "Want to read " << remain << " bytes and actually read " << result <<" bytes " << std::endl;
                    return NULL;
                }
                //对于文本消息来说,这里必须要加一个这个
                pPayload[remain] = '\0';
                return pFullMsg;
            }
            break;

            default:
            std::cout << "Unkown Message type!" << std::endl;
            break;

        }
    }

    return NULL;
}

SimpleChat::SimpleChat(int port): m_Port(port) , m_SelfSession(STDIN_FILENO), m_Select()
{

}

SimpleChat::~SimpleChat()
{
}

int SimpleChat::AddSession(int fd)
{
    GuestSession *session = new GuestSession(fd);
	if (!session)
	{
		return -1;
	}

	m_Guests.push_back(SessionPair{fd, session});
    return 0;
}

int SimpleChat::RemoveSession(SessionPair &session)
{
	GuestsIter it = std::find(m_Guests.begin(), m_Guests.end(), session);
	if (it == m_Guests.end())
		return -1;


	return 0;
}

int SimpleChat::CloseSession(GuestSession *session)
{
    int tempFD = session->GetFileDescr();
    //prevent from multi closing, also close STDIN(0)
    assert(!(tempFD < 0));
    int ret = session->CloseSession();
    if(ret < 0)
    {
        std::cerr << errno << "  " << strerror(errno) << std::endl;
    }
    return ret;
}

GuestSession *SimpleChat::FindSessionByFD(int fd)
{
    GuestsIter it = std::find_if(m_Guests.begin(), m_Guests.end(), [fd](GuestSession *session)
    {
            return session->GetFileDescr() == fd;
    });

    return it != m_Guests.end() ? *it : NULL;
}

SimpleChat::GuestsIter SimpleChat::FindSessionByName(const std::string &name)
{
    GuestsIter it = std::find_if(m_Guests.begin(), m_Guests.end(), [name](GuestSession *session)
    {
            return session->GetName() == name;
    });
    return it;
}

SimpleChat::GuestsIter SimpleChat::FindSessionByID(long sid)
{
    GuestsIter it = std::find_if(m_Guests.begin(), m_Guests.end(), [sid](GuestSession *session)
    {
            return session->GetSessionID() == sid;
    });
    return it;
}



//int Selector::RegisterListener(int fd, IListener *pListener)
//{
//
//	int ret;
//	epoll_event ev;
//	bzero(&ev, sizeof(ev));
//	ev.events = EPOLLIN | EPOLLRDHUP;
//	ev.data.ptr = static_cast<void*>(pListener);
//	ret = epoll_ctl(m_hRoot, EPOLL_CTL_ADD, fd, &ev);
//
//	return 0;
//}
//
//int Selector::UnRegisterListener(int fd, IListener *pListener)
//{
//	int ret;
//	epoll_event ev;
//	bzero(&ev, sizeof(ev));
//	ev.data.fd = fd;
//	ret = epoll_ctl(m_hRoot, EPOLL_CTL_DEL, fd, &ev);
//
//	return 0;
//}
//
//int Selector::Init()
//{
//	m_hRoot = epoll_create(1);
//	if (m_hRoot < 0)
//	{
//		std::cerr << "Epoll create err num: " << errno << strerror(errno) << std::endl;
//		return -1;
//	}
//	else
//	{
//		return 0;
//	}
//}
//
//int Selector::Start()
//{
//	m_bStart = true;
//	//..set pthread_attr 16k
//	int ret = pthread_create(&m_pThread, NULL, StartThread, this);
//	//...handle ret..
//	if (ret < 0)
//		m_bStart = false;
//	return ret;
//}
//
//int Selector::Stop()
//{
//	int ret = 0;
//	if (m_bStart)
//	{
//		ret = pthread_join(m_pThread, NULL);
//		m_bStart = false;
//		assert(m_hRoot >= 0);
//		//...全部EPOLL_CTL_DEL
//		ret = close(m_hRoot);
//		m_hRoot = -1;
//	}
//	return ret;
//}
//

ChatHost::ChatHost(int port) : m_hListenFD(-1), SimpleChat(port)
{

}

int ChatHost::Init(const char *pName)
{
    int ret;
    std::string name(pName);
    m_SelfSession.SetName(name);
	m_hEpollRoot = epoll_create(1);
	if (m_hEpollRoot < 0)
	{
		std::cerr << "Epoll create err num: " << errno << strerror(errno) << std::endl;
		return -1;
	}
	else
	{
		return 0;
	}

    return 0;
}


int ChatHost::Start()
{
    int ret;
}

void *ChatHost::EpollThread(void *param)
{
	int nReady, ret;
	ChatHost *pServer = static_cast<ChatHost*>(param);
	
	epoll_event events[1024];
	SessionPair *pSession;

	while (pServer->m_bStart)
	{
		nReady = epoll_wait(pServer->m_hEpollRoot, events, 1024, -1);
		if (nReady < 0)
		{
			return NULL;
		}
		for (int i = 0; i < nReady; ++i)
		{
			pSession = static_cast<SessionPair*>(events[i].data.ptr);
			if (pSession->fd == pServer->m_hListenFD && events[i].events == EPOLLIN)
			{
				sockaddr_in cliAddr;
				socklen_t cliLen;
				int cliFD = accept(pServer->m_hListenFD, (sockaddr *)&cliAddr, &cliLen);
				if (cliFD < 0)
				{
					//
				}
				//GuestAttr
				pServer->AddSession(cliFD);
			}
			else if (events[i].events == EPOLLIN)
			{
				//pSession->session->HandleMsg();
			}
			else if (events[i].events == EPOLLHUP)
			{
				pServer->RemoveSession(*pSession);
			}
		}
	}

	return nullptr;
}

int ChatHost::HandleMsg(SimpleMsgHdr *pMsg, GuestSession *pSender)
{
    assert( this );

    assert(pSender);
    int ret;
    std::stringstream contents;
    int lenth;
    if(!pMsg)
        return HANDLEMSGRESULT_DELSESSION;


    switch(pMsg->ID)
    {
        case SPLMSG_LOGIN:
        {
            NormalMessage *pLoginMsg = static_cast<NormalMessage*>(pMsg);
            std::string name(pLoginMsg->Payload);
            GuestsIter position = FindSessionByName(name);
            if(position != m_Guests.end())
            {
                contents << "Name :'" << name << "' has existed!";
                lenth = contents.str().size();
                NormalMessage *pErrMsg = (NormalMessage *)(malloc(sizeof(SimpleMsgHdr) + lenth));
                if(!pErrMsg)
                {
                    //out of memory...
                    return -1;
                }

                //组消息要写一个函数...
                pErrMsg->FrameHead = MSG_FRAME_HEADER;
                pErrMsg->Length = lenth;
                pErrMsg->ID = SPLMSG_ERR;

                memcpy(pErrMsg->Payload, contents.str().c_str(), lenth);
                ret = pSender->PostMessage(pErrMsg);
                free(pErrMsg);

                ret = CloseSession(pSender);
                //.....What ever, erase this it
                ret = HANDLEMSGRESULT_DELSESSION;
            }
            else
            {
                //一条单独的SPLMSG_OK，一条群发welcome
                pSender->SetName(name);

                char *pData = new char[len];
                if( pData )
                {
                    MSG_TEXT *pTextMsg= new( pData ) MSG_TEXT( length );
                    send();
                    //pTextMsg->~MSG_TEXT();
                    delete pTextMsg;
                }

                //....SPLMSG_OK
                contents << "Welcome " << name << " add in!";
                lenth = contents.str().size();
                NormalMessage *pOkMsg = (NormalMessage *)(malloc(sizeof(SimpleMsgHdr) + lenth));
                if(!pOkMsg)
                {
                    return -1;
                }
                pOkMsg->FrameHead = MSG_FRAME_HEADER;
                pOkMsg->Lenth = lenth;
                pOkMsg->ID = SPLMSG_LOGIN_OK;
                memcpy(pOkMsg->Payload, contents.str().c_str(), lenth);

                for(GuestsIter it = m_Guests.begin(); it != m_Guests.end(); ++it)
                {
                    ret = (*it)->PostMessage(pOkMsg);
                }
                free(pOkMsg);
            }
        }
        break;
        case SPLMSG_TEXT:
        {
            for(GuestsIter it = m_Guests.begin(); it != m_Guests.end(); ++it)
            {
                ret = (*it)->PostMessage(pMsg);
            }
            free(pMsg);
        }
        break;
    }
    return ret;
}


ChatGuest::ChatGuest(int port) : m_HostIP(0), m_hSocket(-1), SimpleChat(port)
{

}

int ChatGuest::Create(int argc, char **argv)
{
    assert(argv);
    int ret = SimpleChat::Create(argc, argv);
    if(ret < 0)
        return ret;
    assert( argc > 2 );
    assert(argv[2]);
    ret = inet_pton(AF_INET, argv[2], (void*)&m_HostIP);
    if(ret != 1)
    {
        std::cout << "Invaild IP address!Please enter IPV4 addr!" << std::endl;
        return -1;
    }
    return 0;
}

int ChatGuest::Init()
{
    m_hSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_hSocket < 0)
    {
        std::cerr << errno << "  " << strerror(errno) << std::endl;
        return m_hSocket;
    }
    return 0;
}

int ChatGuest::Run()
{
    int ret;
    ret = ConnectHost();
    if(ret < 0)
        return ret;
    ret = Login();
    /*
     * ......
    */

    return 0;
}

int ChatGuest::HandleMsg(SimpleMsgHdr *pMsg, GuestSession *pSender)
{
    assert(pSender);
    int ret;
    std::stringstream contents;

    switch(pMsg->ID)
    {
        case SPLMSG_TEXT:
        {

        }
        break;

        case SPLMSG_LOGIN_OK:
        {

        }
        break;

        case SPLMSG_ERR:
        {

        }
        break;

        default:
        break;
    }
}

int ChatGuest::ConnectHost()
{
    int ret;
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(m_Port);
    servaddr.sin_addr.s_addr = htonl(m_HostIP);

    assert(m_hSocket != -1);

    ret = connect(m_hSocket, (sockaddr *)&servaddr, sizeof(servaddr));
    if(ret < 0)
    {
        std::cerr << errno << "  " << strerror(errno) << std::endl;
        return ret;
    }
    ret = AddSession(m_hSocket);
}

int ChatGuest::Login()
{
    GuestsIter pSelf = FindSessionByFD(STDIN_FILENO);
    GuestsIter pHost = FindSessionByFD(m_hSocket);
    assert(pSelf!=m_Guests.end() && pHost != m_Guests.end());
    NormalMessage *pMsg = (NormalMessage *)malloc(sizeof(SimpleMsgHdr) + (*pSelf)->GetName().size());
    if(!pMsg)
        return -1;
    pMsg->FrameHead = MSG_FRAME_HEADER;
    pMsg->ID = SPLMSG_LOGIN;
    pMsg->Lenth = (*pSelf)->GetName().size();
    memcpy(pMsg->Payload, (*pSelf)->GetName().c_str(), pMsg->Lenth);
    std::cout << "Login payload: " << pMsg->Payload << std::endl;
    (*pHost)->PostMessage(pMsg);
    SimpleMsgHdr *pOkMsg = (*pSelf)->RecvMessage();
    HandleMsg(pOkMsg, *pHost);

}
