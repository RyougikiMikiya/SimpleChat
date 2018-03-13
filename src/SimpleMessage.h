#ifndef __SIMPLE_MESSAGE_H 
#define __SIMPLE_MESSAGE_H

#include <unistd.h>
#include <cassert>
#include <errno.h>
#include <stdint.h>

#define MSG_FRAME_HEADER 0x3154


#define HANDLEMSGRESULT_DELSESSION        101
#define HANDLEMSGRESULT_NAMEHASEXIST      102
#define HANDLEMSGRESULT_LOGINAUTHSUCCESS  103


enum MessageID
{
    SPLMSG_OK = 0xCAFE,
    SPLMSG_LOGIN,
    SPLMSG_LOGIN_OK,
    SPLMSG_CHAT,
    SPLMSG_ERR,
    SPLMSG_BROaDCAST
};

enum ErrMsgType
{
    ERRMSG_NAMEXIST = 0xA0,
};


#pragma pack(1)

struct SimpleMsgHdr
{
    int32_t FrameHead;
    uint16_t ID;
    int32_t Length;

protected:
    SimpleMsgHdr( MessageID msgID, int len) : FrameHead(MSG_FRAME_HEADER), Length(len - sizeof(SimpleMsgHdr))
    {
        assert(Length >=0 );
        ID = static_cast< uint16_t >( msgID );
    }
};

struct LoginMessage : public SimpleMsgHdr
{
    LoginMessage (int payloadLen) : SimpleMsgHdr( SPLMSG_LOGIN , payloadLen + sizeof(*this)){}
    char Payload[0];
};

struct ChatMessage : public SimpleMsgHdr
{
    ChatMessage (int payloadLen) : SimpleMsgHdr( SPLMSG_CHAT , payloadLen + sizeof(*this)){}
    char Payload[0];

};

struct ErrMessage : public SimpleMsgHdr
{
    ErrMessage (ErrMsgType err) : SimpleMsgHdr( SPLMSG_ERR , sizeof(*this))
    {
        Errtype = static_cast< uint8_t >( err );
    }
    uint8_t Errtype;
};

struct LoginOkMessage : public SimpleMsgHdr
{
    LoginOkMessage(int payloadLen) : SimpleMsgHdr(SPLMSG_LOGIN_OK ,payloadLen + sizeof(*this)){}
    char Payload[0];
};


#pragma pack()


#endif
