#ifndef __SIMPLE_MESSAGE_H 
#define __SIMPLE_MESSAGE_H

#include <unistd.h>
#include <errno.h>

#define MSG_FRAME_HEADER 0x3154

enum MessageID
{
    SPLMSG_OK = 0xCAFE,
    SPLMSG_LOGIN,
    SPLMSG_LOGIN_OK,
    SPLMSG_TEXT,
    SPLMSG_ERR
};

#pragma pack(1)

struct SimpleMessage
{
    int FrameHead;
    MessageID ID;
    const int Length;

protected:
    SimpleMessage( MessageID msgID, int len ) : Length( len )
    {
        ID = static_cast< UINT16 >( msgID );
    }
};

struct LoginMessage : public SimpleMessage
{
    UINT32 ItemCount;
    ITEM   Item[0];
    char* Name;

    static const LoginMessage* Unｐａｃｋ( );

    NormalMessage () : SimpleMessage( SPLMSG_LOGIN, sizeof( *this ) ){}
};

#pragma pack()


#endif
