#ifndef __SIMPLE_MESSAGE_H 
#define __SIMPLE_MESSAGE_H

#include <unistd.h>
#include <cassert>
#include <errno.h>
#include <stdint.h>

#include "SimpleUtility.h"

#define MSG_FRAME_HEADER 0x3154


#define HANDLEMSGRESULT_DELSESSION        101
#define HANDLEMSGRESULT_NAMEHASEXIST      102
#define HANDLEMSGRESULT_LOGINAUTHSUCCESS  103


#define BUF_MAX_LEN 2048
#define NAME_MAX_LEN 256

enum MessageID
{
    SPLMSG_OK = 0xCAFE,
    SPLMSG_LOGIN,
    SPLMSG_LOGIN_OK,
    SPLMSG_CHAT,
    SPLMSG_ERR,
    SPLMSG_BROADCAST
};

enum ErrMsgType
{
    ERRMSG_NAMEXIST = 0xA0,
};


struct SimpleMsgHdr
{
public:
    int32_t FrameHead;
    uint16_t ID;
    int16_t Length;

protected:
    SimpleMsgHdr( MessageID msgID, int len) : FrameHead(MSG_FRAME_HEADER), Length(len - sizeof(SimpleMsgHdr))
    {
        assert(Length >=0 );
        assert((Length +sizeof(*this)) < BUF_MAX_LEN );
        ID = static_cast< uint16_t >( msgID );
    }
};

struct LoginMessage : public SimpleMsgHdr
{
public:
    LoginMessage (int16_t payloadLen) : SimpleMsgHdr( SPLMSG_LOGIN , payloadLen + sizeof(*this) - sizeof(std::string))
    {
    // do nothings
    }

    static LoginMessage *Pack(void *pSendBuf,const AuthInfo &info);
    static void Unpack(const SimpleMsgHdr *pRecvBuf, AuthInfo &info);

    AuthInfo Info;
};

struct ChatMessage : public SimpleMsgHdr
{
public:
    ChatMessage (int16_t payloadLen) : SimpleMsgHdr( SPLMSG_CHAT , payloadLen + sizeof(*this))
    {
    // do nothings
    }
    char Contents[0];
};

struct BroadcastMsg : public SimpleMsgHdr
{
public:
    BroadcastMsg(int16_t len) : SimpleMsgHdr( SPLMSG_BROADCAST , len + sizeof(*this))
    {
    // do nothings
    }
    char Contents[0];
};


struct ErrMessage : public SimpleMsgHdr
{
public:
    ErrMessage (ErrMsgType ERR) : SimpleMsgHdr( SPLMSG_ERR , sizeof(*this))
    {
        Errtype = static_cast< uint8_t >( ERR );
    }
    uint8_t Errtype;
};

struct LoginOkMessage : public SimpleMsgHdr
{
public:
    LoginOkMessage(int len) : SimpleMsgHdr(SPLMSG_LOGIN_OK ,len + sizeof(*this))
    {
    // do nothings
    }
    char HostName[0];
};


//return 0 if pBuf is not enough. return -1 fd is invaild. return -2 peer close?
//success return headerLenth + payloadLenth
int RecevieMessage(int fd, void *pBuf, int bufLen);

int PostMessage(int fd,const SimpleMsgHdr*pMsg);

void PutOutMsg(const SimpleMsgHdr *pMsg);

#endif
