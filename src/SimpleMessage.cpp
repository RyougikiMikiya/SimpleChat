#include <iostream>
#include <sstream>

#include <cstdio>
#include <cstring>
#include <cerrno>

#include <cassert>
#include "SimpleUtility.h"
#include "SimpleMessage.h"


int SendMessage(int fd,const SimpleMsgHdr *pMsg)
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

int RecevieMessageOLD(int fd, void *pBuf, int bufLen)
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

int RecevieMessage(int fd, SimpleMsgHdr **pMsg)
{
    assert(fd >= 0);
    int hdrLenth = sizeof(SimpleMsgHdr);
    char headBuf[hdrLenth];
    char *pTmp, *pFull;
    int remain, result;
    SimpleMsgHdr *pHeader;
    int ret = 0;

    remain = hdrLenth;
    result = readn(fd, headBuf, hdrLenth);
    if(result == 0)
    {
        std::cout << "peer close" << std::endl;
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

    pHeader = reinterpret_cast<SimpleMsgHdr*>(headBuf);
    if(pHeader->FrameHead != MSG_FRAME_HEADER)
    {
        std::cout << "Message frame head is incorrect!" << std::endl;
        goto ERR;
    }
    remain = pHeader->Length;
    pFull = new char[hdrLenth + remain];
    if(!pFull)
    {
        std::cout << "allocate memory failed" << std::endl;
        goto ERR;
    }
    pTmp = PutInStream(pFull, *pHeader);

    if(remain > 0)
    {
        result = readn(fd, pTmp, remain);
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

    *pMsg = reinterpret_cast<SimpleMsgHdr*>(pFull);

    return result + hdrLenth;

    ERR:
    if(ret == -1)
    {
        perror("recv msg fail!\n");
    }
    return ret;
}

void DestoryRecvMessage(const SimpleMsgHdr *pMsg)
{
    assert(pMsg);
    const char *pTmp = reinterpret_cast<const char*>(pMsg);
    delete [] pTmp;
}


LoginMessage *LoginMessage::Pack(void *pSendBuf,const AuthInfo &info)
{
    assert(pSendBuf);
    assert(info.UserName.length() < NAME_MAX_LEN);
    int totalLen = info.UserName.length();

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
    const SimpleMsgHdr *pHead = reinterpret_cast<const SimpleMsgHdr*>(pRecvBuf);
    assert(pHead->FrameHead == MSG_FRAME_HEADER);
    assert(pHead->ID == SPLMSG_LOGIN);
    assert(pHead->Length > 0);
    //skip header
    pRecvBuf += 1;

    int totalLen = pHead->Length;

    int nameLen;
    const char *pTmp = GetInStream(pRecvBuf, &nameLen);
    totalLen -= sizeof(int);

    info.UserName.insert(0, pTmp, nameLen);
    totalLen -= nameLen;
    pTmp += nameLen;

    assert(totalLen == 0);
}

UserAttrMsg *UserAttrMsg::Create(UserConstIt first, UserConstIt last)
{
    assert(first != last);

    int totalLen = 0, curLen = 0;
    UserAttr attr;
    int num = 0;

    for(UserConstIt it = first; it != last; ++it)
    {
        attr = it->second;
        curLen = sizeof(attr.UID) + sizeof(attr.bOnline) + sizeof(int) + attr.UserName.length();
        totalLen += curLen;
        num++;
    }

    char *pTmp = new char[totalLen + sizeof(int) + sizeof(SimpleMsgHdr)];
    UserAttrMsg *pMsg = new(pTmp) UserAttrMsg(totalLen, num);
    //point to attrs[0]
    pTmp += (sizeof(SimpleMsgHdr) + sizeof(int));

    for(UserConstIt it = first; it != last; ++it)
    {
        attr = it->second;
        pTmp = PutInStream(pTmp, attr.UID);
        pTmp = PutInStream(pTmp, attr.bOnline);
        pTmp = PutStringInStream(pTmp, attr.UserName.c_str(), attr.UserName.length());
    }

    return pMsg;
}

void UserAttrMsg::Unpack(const UserAttrMsg *pHead, UserList &list)
{
    assert(pHead);
    assert(pHead->FrameHead == MSG_FRAME_HEADER);
    assert(pHead->ID == SPLMSG_USERATTR);
    assert(pHead->Length > 0);

    int num = pHead->Num;
    int attrLen = 0;
    int totalLen = pHead->Length - sizeof(int);

    //skip header
    const char *pTmp = reinterpret_cast<const char*>(pHead);
    pTmp += (sizeof(SimpleMsgHdr) + sizeof(int));

    UserAttr attr;

    for(int i = 0; i < num; i++)
    {
        pTmp = GetInStream(pTmp, &attr.UID);
        attrLen = sizeof(attr.UID);

        pTmp = GetInStream(pTmp, &attr.bOnline);
        attrLen += sizeof(attr.bOnline);

        int nameLen;
        pTmp = GetInStream(pTmp, &nameLen);
        attrLen += sizeof(int);

        attr.UserName.clear();
        attr.UserName.insert(0, pTmp, nameLen);
        pTmp += nameLen;
        attrLen += nameLen;

        list.insert({attr.UID, attr});
        totalLen -= attrLen;
    }

    assert(totalLen == 0);

}

void UserAttrMsg::Destory(UserAttrMsg *pHead)
{
    assert(pHead);
    char *pTmp = reinterpret_cast<char *>(pHead);
    pHead->~UserAttrMsg();
    delete [] pTmp;
}

void ServerChatMsg::SetTimeStamp(ServerChatMsg *pSendBuf, time_t time)
{
    assert(pSendBuf);
    assert(pSendBuf->FrameHead == MSG_FRAME_HEADER);
    assert(pSendBuf->ID == SPLMSG_CHAT);

    SimpleMsgHdr *pHead = static_cast<SimpleMsgHdr*>(pSendBuf);
    pHead += 1;
    PutInStream(pHead, time);
}

SimpleMsgHdr *ServerChatMsg::Pack(void *pSendBuf, const ServerText &sText)
{
    assert(pSendBuf);
    int len = sText.content.length();
    assert(len < BUF_MAX_LEN);

    ServerChatMsg msg(len);
    SimpleMsgHdr *pHeader = &msg;

    char *pTmp = reinterpret_cast<char *>(pSendBuf);

    pTmp = PutInStream(pTmp, *pHeader);
    pTmp = PutInStream(pTmp, sText.Time);
    pTmp = PutInStream(pTmp, sText.UID);
    pTmp = PutStringInStream(pTmp, sText.content.c_str(), static_cast<int>(sText.content.length()));
    SimpleMsgHdr *pFull = reinterpret_cast<SimpleMsgHdr*>(pSendBuf);
    return pFull;
}

void ServerChatMsg::Unpack(const SimpleMsgHdr *pRecvBuf, ServerText &sText)
{
    assert(pRecvBuf);
    const SimpleMsgHdr *pHead = reinterpret_cast<const SimpleMsgHdr*>(pRecvBuf);
    assert(pHead->FrameHead == MSG_FRAME_HEADER);
    assert(pHead->ID == SPLMSG_CHAT);
    assert(pHead->Length > 0);

    pRecvBuf += 1;

    int totalLen = pHead->Length;

    const char *pTmp = GetInStream(pRecvBuf, &sText.Time);
    totalLen -= sizeof(sText.Time);

    pTmp = GetInStream(pTmp, &sText.UID);
    totalLen -= sizeof(sText.UID);

    int contentLen;
    pTmp = GetInStream(pTmp, &contentLen);
    totalLen -= sizeof(int);

    sText.content.insert(0, pTmp, contentLen);
    totalLen -= contentLen;

    assert(totalLen == 0);
}

SimpleMsgHdr *UserInputMsg::Pack(void *pSendBuf, const ClientText &cText)
{
    assert(pSendBuf);
    int len = cText.content.length();
    assert(len < BUF_MAX_LEN);

    UserInputMsg msg(len);
    SimpleMsgHdr *pHeader = &msg;

    char *pTmp = reinterpret_cast<char *>(pSendBuf);

    pTmp = PutInStream(pTmp, *pHeader);
    pTmp = PutInStream(pTmp, cText.UID);
    pTmp = PutStringInStream(pTmp, cText.content.c_str(), static_cast<int>(cText.content.length()));
    SimpleMsgHdr *pFull = reinterpret_cast<SimpleMsgHdr*>(pSendBuf);
    return pFull;
}

void UserInputMsg::Unpack(const SimpleMsgHdr *pRecvBuf, ClientText &cText)
{
    assert(pRecvBuf);
    const SimpleMsgHdr *pHead = reinterpret_cast<const SimpleMsgHdr*>(pRecvBuf);
    assert(pHead->FrameHead == MSG_FRAME_HEADER);
    assert(pHead->ID == SPLMSG_USERINPUT);
    assert(pHead->Length > 0);

    pRecvBuf += 1;

    int totalLen = pHead->Length;

    const char *pTmp = GetInStream(pRecvBuf, &cText.UID);
    totalLen -= sizeof(cText.UID);

    int contentLen;
    pTmp = GetInStream(pTmp, &contentLen);
    totalLen -= sizeof(int);

    cText.content.insert(0, pTmp, contentLen);
    totalLen -= contentLen;

    assert(totalLen == 0);
}

LoginNoticeMsg *LoginNoticeMsg::Pack(void *pSendBuf, const UserAttr &attr)
{
    assert(pSendBuf);
    assert(attr.UserName.length()!= 0);
    LoginNoticeMsg msg(attr.UserName.length());
    SimpleMsgHdr *pHeader = &msg;
    char *pTmp = reinterpret_cast<char*>(pSendBuf);

    pTmp = PutInStream(pTmp, *pHeader);
    pTmp = PutInStream(pTmp, attr.UID);
    pTmp = PutInStream(pTmp, attr.bOnline);
    pTmp = PutStringInStream(pTmp, attr.UserName.c_str(), attr.UserName.length());
    LoginNoticeMsg *pFull = reinterpret_cast<LoginNoticeMsg*>(pSendBuf);

    return pFull;
}

void LoginNoticeMsg::Unpack(const SimpleMsgHdr *pMsgHeader, UserAttr &attr)
{
    assert(pMsgHeader);
    assert(pMsgHeader->FrameHead == MSG_FRAME_HEADER);
    assert(pMsgHeader->ID == SPLMSG_LOGIN_NOTICE);
    assert(pMsgHeader->Length > 0);

    int totalLen = pMsgHeader->Length;

    //skip header
    pMsgHeader += 1;

    const char *pTmp = reinterpret_cast<const char*>(pMsgHeader);
    pTmp = GetInStream(pTmp, &attr.UID);
    totalLen -= sizeof(sizeof(attr.UID));

    pTmp = GetInStream(pTmp, &attr.bOnline);
    totalLen -= sizeof(attr.bOnline);

    int nameLen;
    pTmp = GetInStream(pTmp, &nameLen);
    totalLen -= sizeof(int);

    attr.UserName.insert(0, pTmp, nameLen);
    pTmp += nameLen;
    totalLen -= nameLen;

    assert(totalLen == 0);
}
