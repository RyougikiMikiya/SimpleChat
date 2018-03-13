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


static long sSessionID = 0;

SimpleSession::SimpleSession(int fd) : m_FD(fd), m_SessionID(sSessionID++)
{

}

SimpleSession::~SimpleSession()
{

}

int SimpleSession::PostMessage(SimpleMsgHdr *pMsg)
{
    int res;
    if(m_FD == STDIN_FILENO)
    {
        ChatMessage *pFullMsg = static_cast<ChatMessage*>(pMsg);
        pFullMsg->Payload[pFullMsg->Length] = '\0';
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
        ChatMessage *pMsg = new(m_RecvMsgBuf) ChatMessage(line.size());
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
    assert(pHandler);
    ChatHost *pServer = static_cast<ChatHost*>(pHandler);
    SimpleMsgHdr *pMsg = RecvMessage();
    if(!pMsg)
        return -1;
    return HandleMsgByHost(pServer, pMsg);
}

int SessionInHost::HandleMsgByHost(ChatHost *pHandler, SimpleMsgHdr *pMsg)
{
    assert(pMsg && pHandler);
    int ret;
    std::stringstream contents;
    int length;

    switch(pMsg->ID)
    {
    case SPLMSG_LOGIN:
    {
        LoginMessage *pLoginMsg = reinterpret_cast<LoginMessage *>(pMsg);
        std::string name(pLoginMsg->Payload);
        AuthenInfo info(name);
        std::cout << name << " request login!" << std::endl;
        if(!pHandler->LoginAuthentication(info))
        {
            std::cout << name << " has existed, send err msg!" << std::endl;
            ErrMessage errmsg(ERRMSG_NAMEXIST);
            ret = PostMessage(&errmsg);
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
            ret = PostMessage(pLoginOk);
            pLoginOk->~LoginOkMessage();
            if(ret < 0)
                return -1;

            contents.clear();
            contents.str("");

            contents << "Welcome " << name << " add in!";
            length = contents.str().size();

            //后面用Broadcast消息替换
            ChatMessage *pWelcome = new(m_SendMsgBuf) ChatMessage(length);
            memcpy(pWelcome->Payload, contents.str().c_str(), length);
            pHandler->PushToAll(pWelcome);
            pWelcome->~ChatMessage();
        }
    }
        break;
    case SPLMSG_CHAT:
    {
        ChatMessage *pText = static_cast<ChatMessage*>(pMsg);
        pHandler->PushToAll(pText);
    }
        break;
    default:
        std::cout << "Unkown Message Type!" << std::endl;
        break;

    }
    return ret;
}

int SessionInGuest::OnRecvMessage(SimpleChat *pHandler)
{
    assert(this);
    assert(pHandler);
    ChatGuest *pClient = static_cast<ChatGuest*>(pHandler);
    SimpleMsgHdr *pMsg = RecvMessage();
    if(!pMsg)
        return -1;
    return HandleMsgByGuest(pClient, pMsg);
}

int SessionInGuest::HandleMsgByGuest(ChatGuest *pHandler, SimpleMsgHdr *pMsg)
{
    assert(pHandler);
    assert(pMsg->FrameHead == MSG_FRAME_HEADER);
    assert(pMsg->Length < 2048);
    char tmpBuf[2048];

    switch(pMsg->ID)
    {
    case SPLMSG_LOGIN_OK:
    {
        LoginOkMessage *pLoginOk = static_cast<LoginOkMessage *>(pMsg);
        memcpy(tmpBuf, pLoginOk->Payload, pLoginOk->Length);
        tmpBuf[pLoginOk->Length] = '\0';
        SetName(std::string(tmpBuf));
        return HANDLEMSGRESULT_LOGINAUTHSUCCESS;
    }
        break;
    case SPLMSG_CHAT:
    {
        ChatMessage *pText = reinterpret_cast<ChatMessage*>(pMsg);
        pText->Payload[pText->Length] = '\0';
        std::cout << pText->Payload << std::endl;
    }
        break;
    case SPLMSG_ERR:
    {
        ;
    }
        break;
    default:
        std::cout << "Unkown Message Type!" << std::endl;
        break;
    }

    return 0;
}
