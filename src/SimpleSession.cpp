#include <iostream>
#include <algorithm>
#include <sstream>
#include <utility>
#include <cerrno>
#include <cstring>
#include <cassert>

#include <unistd.h>

#include "SimpleUtility.h"
#include "SimpleSession.h"


static long SimpleID = 0;

SimpleSession::SimpleSession(int fd) : m_FD(fd), m_SessionID(-1)
{

}

SimpleSession::~SimpleSession()
{
    assert(m_FD < 0);
}

int SimpleSession::PostMessage(SimpleMsgHdr *pMsg)
{
    int res;
    if(m_FD == STDIN_FILENO)
    {
        TextMessage *pFullMsg = static_cast<TextMessage*>(pMsg);
        std::cout << pFullMsg->Payload << std::endl;
        return 0;
    }
    else
    {
        res = writen(m_FD, (char*)pMsg, sizeof(SimpleMsgHdr) + pMsg->Length);
        if(res < 0)
        {
            std::cerr << "Write err: " << errno << "  " << strerror(errno) << std::endl;
            return -1;
        }
    }
    return res;
}

SimpleMsgHdr *SimpleSession::RecvMessage()
{
    int hdrLenth = sizeof(SimpleMsgHdr);
    int remain, result;

    if(m_FD == STDIN_FILENO)
    {
        std::string line;
        std::getline(std::cin, line);
        if(line.size() > 1024)
        {
            std::cout << "Message is too long" << std::endl;
            return NULL;
        }
        TextMessage *pMsg = new(m_RecvMsgBuf) TextMessage();
        pMsg->FrameHead = MSG_FRAME_HEADER;
        pMsg->ID = SPLMSG_TEXT;
        pMsg->Length = line.size();
        memcpy(pMsg->Payload, line.c_str(), line.size());
        return pMsg;
    }
    else
    {
        remain = hdrLenth;
        result = readn(m_FD, m_RecvMsgBuf, remain);
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

        SimpleMsgHdr* pMsg= reinterpret_cast<SimpleMsgHdr*>(m_RecvMsgBuf);
        if(pMsg->FrameHead != MSG_FRAME_HEADER)
        {
            std::cout << "Message frame head is incorrect!" << std::endl;
            return NULL;
        }
        remain = pMsg->Length;
        if(remain > 0)
        {
            result = readn(m_FD, m_RecvMsgBuf + hdrLenth, remain);
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
        }

        return pMsg;
    }

    return NULL;
}


int SessionInHost::OnRecvMessage(SimpleChat *pHandler)
{
    assert(pServer);
    ChatHost *pServer = dynamic_cast<ChatHost*>(pHandler);
    SimpleMsgHdr *pMsg = RecvMessage();
    if(!pMsg)
        return -1;
    return HandleMsgByHost(pServer, pMsg);
}

int SessionInHost::HandleMsgByHost(ChatHost *pHandler, SimpleMsgHdr *pMsg)
{
    int ret;
    std::stringstream contents;
    int length;
    assert(pMsg);

    switch(pMsg->ID)
    {
        case SPLMSG_LOGIN:
        {
            LoginMessage *pLoginMsg = reinterpret_cast<LoginMessage *>(pMsg);
            std::string name(pLoginMsg->Payload);
            AuthenInfo info(name);
            if(pHandler->LoginAuthentication(info))
            {
                ErrMessage errmsg(ERRMSG_NAMEXIST);
                ret = PostMessage(errmsg);
                ret = HANDLEMSGRESULT_NAMEHASEXIST;
            }
            else
            {
                //一条单独的SPLMSG_OK，一条群发welcome
                SetName(name);

                contents << pHandler->GetSelfName();
                length = contents.str().size();
                LoginOkMessage *pLoginOk = new(m_SendMsgBuf) LoginOkMessage(length);
                memcpy(pLoginOk->Payload, contents.str().c_str(), length);
                PostMessage(pLoginOk);
                pLoginOk->~LoginOkMessage();

                contents.clear();
                contents.str("");

                contents << "Welcome " << name << " add in!";
                length = contents.str().size();
                TextMessage *pWelcome = new(m_SendMsgBuf) TextMessage(length);
                memcpy(pWelcome->Payload, contents.str().c_str(), length);
                pHandler->PushToAll(pWelcome);
                pWelcome->~TextMessage();
            }
        }
        break;
        case SPLMSG_TEXT:
        {
            TextMessage *pText = dynamic_cast<TextMessage*>(pMsg);
            pHandler->PushToAll(pText);
        }
        break;
    }
    return ret;
}

int SessionInGuest::OnRecvMessage(SimpleChat *pHandler)
{
    assert(this);
    assert(pHandler);
    ChatGuest *pClient = dynamic_cast<ChatHost*>(pHandler);
    SimpleMsgHdr *pMsg = RecvMessage();
    if(!pMsg)
        return -1;
    return HandleMsgByGuest(pClient, pMsg);
}

int SessionInGuest::HandleMsgByGuest(ChatGuest *pHandler, SimpleMsgHdr *pMsg)
{
    assert(pHandler);
    return 0;
}
