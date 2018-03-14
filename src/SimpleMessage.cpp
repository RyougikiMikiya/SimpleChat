#include <iostream>

#include <cstring>
#include <cerrno>

#include <cassert>
#include "SimpleUtility.h"
#include "SimpleMessage.h"


int PostMessage(int fd, SimpleMsgHdr *pMsg, char *recvBuf)
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

int RecevieMessage(int fd, char *pBuf, int bufLen)
{
    assert(fd >= 0);
    assert(pBuf);
    assert(bufLen > 0);

    int hdrLenth = sizeof(SimpleMsgHdr);
    int remain, result;
    int ret = 0;
    SimpleMsgHdr *pMsg;
    if(bufLen < hdrLenth)
    {
        std::cerr << "bufLen short than Message header" << std::endl;
        goto ERR;
    }

    remain = hdrLenth;
    result = readn(fd, m_RecvBuf, remain);
    if(result < 0)
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
    if(bufLen < hdrLenth + remain)
    {
        std::cerr << "bufLen short than Message payload" << std::endl;
        goto ERR;
    }
    if(remain > 0)
    {
        result = readn(fd, pBuf + hdrLenth, remain);
        if(result < 0)
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
        std::cerr << "Recv fail: " << errno << "  " << strerror(errno) << std::endl;
    }
    return ret;

}
