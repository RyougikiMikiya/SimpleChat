#ifndef SIMPLECLIENT_H
#define SIMPLECLIENT_H

#include <netinet/in.h>

#include "SimpleMessage.h"
#include "SimpleUtility.h"

class SimpleClient
{
public:
    SimpleClient();

    int Init(const char *pName, const char *pIP, int port) ;
    int Start() ;

private:
    int ConnectHost();
    int Login();

    SimpleMsgHdr *ClinetRecv();


private:
    UserAttr m_selfAttr;
    int m_hSocket;
    int m_Port;
    in_addr_t m_HostIP;

    char m_RecvBuf[2048];
};

#endif // SIMPLECLIENT_H
