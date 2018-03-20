#ifndef __SIMPLE_MESSAGE_H 
#define __SIMPLE_MESSAGE_H

#include <unistd.h>
#include <cassert>
#include <errno.h>
#include <cstdint>

#include "SimpleUtility.h"

#define MSG_FRAME_HEADER 0x3154


#define HANDLEMSGRESULT_DELSESSION        101
#define HANDLEMSGRESULT_NAMEHASEXIST      102
#define HANDLEMSGRESULT_LOGINAUTHSUCCESS  103


#define BUF_MAX_LEN 1024
#define NAME_MAX_LEN 256

enum MessageID
{
    SPLMSG_OK = 0xCAFE,
    SPLMSG_LOGIN,
    SPLMSG_LOGIN_OK,
    SPLMSG_USERINPUT,
    SPLMSG_CHAT,
    SPLMSG_ERR,
    SPLMSG_DATAHEAD,
    SPLMSG_DATAEND,
    SPLMSG_USERATTR
};

enum BroadCastType
{
    BCT_LOGIN = 0x00A0,
};

enum ErrMsgType
{
    ERRMSG_NAMEXIST = 0x00DD,
};

enum DataMsgType
{
    DMT_USERATTR = 0x01A0,
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
        ID = static_cast< uint16_t >( msgID );
        if(ID != SPLMSG_USERATTR)
            assert((Length +sizeof(*this)) < BUF_MAX_LEN );

    }
};

//所有的变长结构体（包含string类)都不要直接使用而使用静态函数传递Pack打包，Unpack解包设置或获取想要的内容

struct LoginMessage : public SimpleMsgHdr
{
    static LoginMessage *Pack(void *pSendBuf,const AuthInfo &info);
    static void Unpack(const SimpleMsgHdr *pRecvBuf, AuthInfo &info);

private:
    //len 表示string所包含字符串长度，下同
    LoginMessage (int16_t len) : SimpleMsgHdr( SPLMSG_LOGIN , len + sizeof(int) + sizeof(SimpleMsgHdr))
    {
    // do nothings
    }

    AuthInfo Info;
};

struct LoginOkMessage : public SimpleMsgHdr
{
public:
    LoginOkMessage(uint64_t uid) : SimpleMsgHdr(SPLMSG_LOGIN_OK , sizeof(*this)), UID(uid)
    {
    // do nothings
    }

public:
    uint64_t UID;
};

struct UserInputMsg : public SimpleMsgHdr
{
public:
    static SimpleMsgHdr *Pack(void *pSendBuf, const ClientText &cText);
    static void Unpack(const SimpleMsgHdr *pRecvBuf, ClientText &cText);


private:
    UserInputMsg (int64_t len) : SimpleMsgHdr( SPLMSG_USERINPUT, len + sizeof(TextContent.UID) + sizeof(int) + sizeof(SimpleMsgHdr) )
    {
        //do nothing
    }

    ClientText TextContent;
};

struct ServerChatMsg : public SimpleMsgHdr
{
public:

    static void SetTimeStamp(ServerChatMsg *pSendBuf, time_t time);
    static SimpleMsgHdr *Pack(void *pSendBuf, const ServerText &sText);
    static void Unpack(const SimpleMsgHdr *pRecvBuf, ServerText &sText);

private:
    ServerChatMsg (int16_t len) : SimpleMsgHdr( SPLMSG_CHAT , len + sizeof(TextContent.Time)
                                                            + sizeof(TextContent.UID) + sizeof(int) + sizeof(SimpleMsgHdr))
    {
        //do nothing
    }

    ServerText TextContent;
};

struct ErrMessage : public SimpleMsgHdr
{
public:
    ErrMessage (ErrMsgType ERR) : SimpleMsgHdr( SPLMSG_ERR , sizeof(*this))
    {
        Errtype = static_cast< uint16_t >( ERR );
    }
    uint16_t Errtype;
};

struct DataMsgHead : public SimpleMsgHdr
{
public:

    DataMsgHead(int sum, DataMsgType type) : SimpleMsgHdr(SPLMSG_DATAHEAD, sizeof(*this)) ,
        Sum(sum)
    {
        //do nothings
        DataType = static_cast< uint16_t >( type );
    }

    int Sum;
    uint16_t DataType;
};

struct UserAttrMsg : public SimpleMsgHdr
{
public:
    static UserAttrMsg *Create(UserConstIt first, UserConstIt last);
    static void Unpack(const UserAttrMsg *pHead, UserList &list);
    static void Destory(UserAttrMsg *pHead);

    int Num;

private:
    //len代表下面这个数组的总长
    UserAttrMsg(int16_t len, int num) : SimpleMsgHdr(SPLMSG_USERATTR, len + sizeof(int) + sizeof(SimpleMsgHdr)) , Num(num)
    {
        //nothing
    }

    UserAttr attrs[0];
};

//return 0 if pBuf is not enough. return -1 fd is invaild. return -2 peer close?
//success return headerLenth + payloadLenth
int RecevieMessage(int fd, void *pBuf, int bufLen);

int RecevieMessage(int fd, SimpleMsgHdr **pMsg);

int SendMessage(int fd,const SimpleMsgHdr*pMsg);

void PutOutMsg(const SimpleMsgHdr *pMsg);

#endif
