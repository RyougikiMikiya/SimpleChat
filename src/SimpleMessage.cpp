#include <iostream>
#include <sstream>

#include <cstdio>
#include <cstring>
#include <cerrno>

#include <cassert>
#include "SimpleUtility.h"
#include "SimpleMessage.h"


int PostMessage(int fd,const SimpleMsgHdr *pMsg)
{
    assert(fd >= 0);
    assert(pMsg);
    int ret = writen(fd, (char*)pMsg, sizeof(SimpleMsgHdr) + pMsg->Length);
    if(ret < 0)
    {
        std::cerr << "Write ERR: " << errno << "  " << strerror(errno) << std::endl;
        ret = -1;
    }

    return ret;
}

int RecevieMessage(int fd, void *pBuf, int bufLen)
{
    assert(fd >= 0);
    assert(pBuf);
    assert(bufLen > 0);

    int hdrLenth = sizeof(SimpleMsgHdr);
    int remain, result;
    int ret = 0;
    SimpleMsgHdr *pMsg;
    char *pRemain;
    if(bufLen < hdrLenth)
    {
        std::cerr << "bufLen short than Message header" << std::endl;
        goto ERR;
    }

    remain = hdrLenth;
    result = readn(fd, pBuf, remain);
    if(result == 0)
    {
        ret = -2;
        goto ERR;
    }
    else if( result < 0 )
    {
        ret = -1;
        goto ERR;
    }
    else if(result != remain)
    {
        ret = -1;
        goto ERR;
    }

    pMsg= reinterpret_cast<SimpleMsgHdr*>(pBuf);
    if(pMsg->FrameHead != MSG_FRAME_HEADER)
    {
        std::cout << "Message frame head is incorrect!" << std::endl;
        goto ERR;
    }
    remain = pMsg->Length;
    pMsg += 1;
    pRemain = reinterpret_cast<char*>(pMsg);
    if(bufLen < hdrLenth + remain)
    {
        std::cerr << "bufLen short than Message payload" << std::endl;
        goto ERR;
    }
    if(remain > 0)
    {
        result = readn(fd, pRemain, remain);
        if(result == 0)
        {
            ret = -2;
            goto ERR;
        }
        else if(result < 0)
        {
            ret = -1;
            goto ERR;
        }
        else if(result != remain)
        {
            ret = -1;
            goto ERR;
        }
    }

    return result + hdrLenth;

    ERR:
    if(ret == -1)
    {
        perror("recv msg fail!\n");
    }
    return ret;

}

void PutOutMsg(const SimpleMsgHdr *pMsg)
{
    assert(pMsg);
    assert(pMsg->FrameHead == MSG_FRAME_HEADER);
    assert(pMsg->Length < BUF_MAX_LEN);
    switch(pMsg->ID)
    {
    case SPLMSG_CHAT :
    {
        const ChatMessage *pFull = static_cast<const ChatMessage*>(pMsg);
        std::cout << pFull->Contents << std::endl;
    }
        break;
    default:
        std::cout << "Unkown Message type" << std::endl;
        break;
    }
}

LoginMessage *LoginMessage::Pack(void *pSendBuf,const AuthInfo &info)
{
    assert(pSendBuf);
    assert(info.UserName.length() < NAME_MAX_LEN);
    int totalLen = info.UserName.length() + sizeof(int);

    LoginMessage msg(totalLen);
    SimpleMsgHdr *pHeader = &msg;

    char *pTmp = reinterpret_cast<char*>(pSendBuf);

    pTmp = PutInStream(pTmp, *pHeader);
    pTmp = PutStringInStream(pTmp, info.UserName.c_str(), static_cast<int>(info.UserName.length()));
    LoginMessage *pFull = reinterpret_cast<LoginMessage*>(pSendBuf);
    return pFull;
}

void LoginMessage::Unpack(const SimpleMsgHdr *pRecvBuf, AuthInfo &info)
{
    assert(pRecvBuf);
    char tmpBuf[1024];
    const SimpleMsgHdr *pHead = reinterpret_cast<const SimpleMsgHdr*>(pRecvBuf);
    assert(pHead->FrameHead == MSG_FRAME_HEADER);
    assert(pHead->ID == SPLMSG_LOGIN);
    assert(pHead->Length > 0);
    pRecvBuf += 1;
    int inLen = pHead->Length;
    int outLen = inLen;
    GetStringInStream(pRecvBuf, tmpBuf, &outLen);
    assert(outLen < NAME_MAX_LEN);
    tmpBuf[outLen] = '\0';
    info.UserName = tmpBuf;
}
