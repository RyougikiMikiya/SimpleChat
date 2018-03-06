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

static long SimpleID = 0;

#define HANDLEMSGRESULT_DELSESSION        -1



GuestSession::GuestSession(int fd) : m_FD(fd), m_SessionID(SimpleID++)
{

}

GuestSession::~GuestSession()
{
    assert(m_FD < 0);
}

int GuestSession::PostMessage(simpleMessage * msg)
{
    int remain = msg->GetMsgLenth();
    char *buf = msg->GetPayload();
    int nbytes;
    while(remain > 0)
    {
        nbytes = write(m_FD, buf, remain);
        if(nbytes < 0)
            return -1;
        remain -= nbytes;
        buf += nbytes;
    }
    return 0;
}

SimpleMessage *GuestSession::RecvMessage()
{
    char buf[sizeof(SimpleMessage)];
    int hdrLenth = sizeof(header);
    if(m_FD != STDIN_FILENO)
    {

    }
    else
    {
        int remain = hdrLenth;
        while(remain > 0)
        {
            int nbytes = read(m_FD, buf, remain);
            if(nbytes < 0)
                return NULL;
            if(nbytes == 0)
                //peer close socket
                return NULL;
            remain -= nbytes;
            buf += nbytes;
        }
        SimpleMessage *header= reinterpret_cast<SimpleMessage*>(buf);
        if(header->FrameHead != MSG_FRAME_HEADER)
        {
            cout << "Message frame head is incorrect!" << endl;
            return NULL;
        }
        remain = header->Lenth;

        switch(header->ID)
        {
            case SPLMSG_LOGIN:
            {
                NormalMessage *body = (NormalMessage *)malloc(hdrLenth + remain);
                if(!body)
                    return NULL;
                nbytes = read(m_FD, (char*)body + hdrLenth, remain);
                //...
                return body;
            }
            break;

            default:
            cout << "Unkown Message type!" << endl;
            break;

        }
    }

    return NULL;
}

/*

*/

SimpleChat::SimpleChat(int port): m_Port(port)
{

}

SimpleChat::~SimpleChat()
{
}

int SimpleChat::AddSession(int fd)
{
    GuestSession *session = new GuestSession(fd);
    if(!session)
    {
        return -1;
    }
    m_Guests.push_back(session);
    return 0;
}

int SimpleChat::CloseSession(GuestSession *session)
{
    int tempFD = session->GetFileDescr();
    //prevent from multi closing, also close STDIN(0)
    assert(!(tempFD < 0));
    int ret = close(tempFD);
    if(ret < 0)
    {
        std::cerr << errno << "  " << strerror(errno) << std::endl;
    }
    return ret;
}

SimpleChat::GuestsIter SimpleChat::FindSessionByFD(int fd)
{
    GuestsIter it = std::find_if(m_Guests.begin(), m_Guests.end(), [fd](GuestSession *session)
    {
            return session->GetFileDescr() == fd;
    });
    return it;
}

SimpleChat::GuestsIter SimpleChat::FindSessionByName(const std::string &name)
{
    GuestsIter it = std::find_if(m_Guests.begin(), m_Guests.end(), [name](GuestSession *session)
    {
            return session->GetName() == name;
    });
    return it;
}

ChatHost::ChatHost(int port) : m_ListenFd(-1), SimpleChat(port)
{

}

int ChatHost::Create(int argc, char **argv)
{
    std::string name(argv[1]);
    int ret = AddSession(STDIN_FILENO);
    if(ret < 0)
        return ret;
    GuestsIter pos = FindSessionByFD(STDIN_FILENO);
    assert(pos != m_Guests.end());
    (*pos)->SetName(name);
    return 0;
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

int ChatHost::Run()
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
            int cliFD = accept(m_ListenFd, (sockaddr*)&cliaddr, &cliLen);
            if (cliFD < 0)
            {
                std::cerr << cliFD << "  " << errno << "  " << strerror(errno) << std::endl;
                continue;
            }
            std::cout << inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, buf, sizeof buf) << std::endl;
            FD_SET(cliFD, &m_FdSet);
            AddSession(cliFD);

            if (cliFD > maxFd)
                maxFd = cliFD;
            --nReady;
            if (nReady == 0)//没有更多的事件了
                continue;
        }

        for(GuestsIter it = m_Guests.begin(); it != m_Guests.end(); ++it)
        {
            tempFd = (*it)->GetFileDescr();
            if(FD_ISSET(tempFd, &rfd))
            {
                simpleMessage *msg = (*it)->RecvMessage();
                if(!msg)
                {
                    //if we fail to recevie msg. erase?
                    continue;
                }
                int result = HandleMsg(msg, *it);
                if(result == HANDLEMSGRESULT_DELSESSION)
                {
                    it = m_Guests.erase(it);
                }
                --nReady;
                if(nReady == 0)
                    break;
            }
        }
    }
}

int ChatHost::HandleMsg(simpleMessage *msg, GuestSession *sender)
{
    assert(sender);
    int ret;
    switch(msg->GetMsgID())
    {
        case SPLMSG_LOGIN:
        {
            std::string name(msg->GetPayload());
            GuestsIter position = FindSessionByName(name);
            if(position != m_Guests.end())
            {
                simpleMessage *errMsg = new simpleMessage(SPLMSG_ERR);
                std::stringstream contents;
                contents << "Name :'" << name << "' has existed!";
                errMsg->AddPayload(contents);
                ret = sender->PostMessage(errMsg);
                ret = CloseSession(sender);
                delete sender;
                return HANDLEMSGRESULT_DELSESSION;
            }
            else
            {
                sender->SetName(name);
                simpleMessage *loginOkMsg = new simpleMessage(SPLMSG_OK);
                sender->PostMessage(loginOkMsg);
            }
        }
        break;
        case SPLMSG_TEXT:
        {
            for(GuestsIter it = m_Guests.begin(); it != m_Guests.end(); ++it)
            {
                ret = (*it)->PostMessage(msg);
            }
        }
        break;
    }
    return 0;
}


