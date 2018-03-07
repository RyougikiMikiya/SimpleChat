#ifndef __SIMPLE_MESSAGE_H 
#define __SIMPLE_MESSAGE_H

#include <unistd.h>
#include <errno.h>

#define MSG_FRAME_HEADER 0x3154

enum MessageID
{
    SPLMSG_OK = 0xCAFE,
    SPLMSG_LOGIN,
    SPLMSG_TEXT,
    SPLMSG_ERR
};




#pragma pack(1)

struct SimpleMessage
{
    int FrameHead;
    MessageID ID;
    int Lenth;
};

struct NormalMessage : public SimpleMessage
{
    char Payload[0];
};

#pragma pack()

#endif
