#ifndef SIMPLECLIENT_H
#define SIMPLECLIENT_H

#include <netinet/in.h>

#include "SimpleListener.h"
#include "SimpleMessage.h"
#include "SimpleUtility.h"

class SimpleClient : public IReceiver
{
public:
    SimpleClient();
    ~SimpleClient(){}

    int Init(const char *pName, const char *pIP, int port) ;
    int Start() ;
//overrides
    void OnReceive();

private:
    int ConnectHost();
    int Login();

    SimpleMsgHdr *Receive();
    int HandleMsg(SimpleMsgHdr *pMsg);

private:
    UserAttr m_SelfAttr;
    int m_hSocket;
    int m_Port;
    in_addr_t m_HostIP;
    SimpleListener m_Listener;
    volatile bool m_bWork;

    char m_SendBuf[BUF_MAX_LEN];
    char m_RecvBuf[BUF_MAX_LEN];
};

#endif // SIMPLECLIENT_H
