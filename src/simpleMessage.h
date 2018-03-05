#ifndef __SIMPLE_MESSAGE_H 
#define __SIMPLE_MESSAGE_H

#include <vector>
#include <sstream>


enum MESSAGEID
{
    SPLMSG_OK = 0xCAFE,
    SPLMSG_LOGIN,
    SPLMSG_TEXT,
    SPLMSG_ERR
};



class simpleMessage
{
public:
    simpleMessage(MESSAGEID ID);
	~simpleMessage();

    void        AddPayload(std::stringstream &payload);

    MESSAGEID   GetMsgID() const {return m_ID;}
    const char* GetPayload() const {return m_Payload.data();}
    int         GetMsgLenth() const {return m_Payload.size();}

private:
    static int FrameHead;
	MESSAGEID m_ID;
    std::vector<char> m_Payload;
};

#endif
