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
    int Start();
    int Stop();
//overrides
    void OnReceive();

private:
    int ConnectHost();
    int Login();
    int ReceiveUserAttr(int sum);

    void PrintMsgToScreen(const SimpleMsgHdr *pMsg);
    std::string FormatClientText(const ServerText &text);

    const SimpleMsgHdr *Receive();
    int Send(SimpleMsgHdr *pMsg)
    {
        return SendMessage(m_hSocket, pMsg);
    }

    int HandleMsg(const SimpleMsgHdr *pMsg);

private:
    class CInputReceiver : public IReceiver
    {
    public:
        CInputReceiver(SimpleClient *pClient) : m_pClient(pClient){}
        void OnReceive();
    private:
        SimpleClient *m_pClient;
        char m_SendBuf[BUF_MAX_LEN];
        std::string m_LineBuf;
    };

private:
    UserAttr m_SelfAttr;
    int m_hSocket;
    int m_Port;
    in_addr_t m_HostIP;
    SimpleListener m_Listener;
    volatile bool m_bWork;

    CInputReceiver m_STDIN;

    //和服务器那边保持一致
    UserList m_Users;

    char m_SendBuf[BUF_MAX_LEN];
    char m_RecvBuf[BUF_MAX_LEN];
};

#endif // SIMPLECLIENT_H
