#ifndef SIMPLESERVER_H
#define SIMPLESERVER_H

#include <string>
#include <vector>

#include "SimpleMessage.h"
#include "SimpleListener.h"
#include "SimpleUtility.h"

class SimpleServer : public IReceiver
{
public:
    SimpleServer();
    ~SimpleServer(){}

    class CSession : public IReceiver
    {
    public:
        CSession(SimpleServer *pHost, int fd);
        ~CSession();
        //overrides
        void OnReceive();
        //function for Server
        int Create();
        int Destory();
        int Send(const SimpleMsgHdr *pMsg)
        {
            assert(this);
            return PostMessage(m_hFD, pMsg);
        }
        const std::string &GetUserName()const
        {
            assert(this);
            return m_Attr.UesrName;
        }

    private:
        //function self
        SimpleMsgHdr *Recevie();
        int HandleMessage(const SimpleMsgHdr *pMsg);
        bool LoginAuthentication(AuthInfo &info);

        SimpleServer *m_pServer;
        int m_hFD;
        UserAttr m_Attr;

        char m_SendBuf[BUF_MAX_LEN];
        char m_RecvBuf[BUF_MAX_LEN];
    };

    int Init(const char *pName, int port);
    int Uninit();

    int Start();
    int Stop();

public:
    void OnReceive();

    //fuction for session
protected:
    void PushToAll(const SimpleMsgHdr *pMsg);
    bool CheckNameExisted(const std::string &name);

    class SInputReceiver : public IReceiver
    {
    public:
        SInputReceiver(SimpleServer *pServer) : m_pServer(pServer){}
        void OnReceive();
    private:
        SimpleServer *m_pServer;
        char m_SendBuf[BUF_MAX_LEN];
    };

private:
    typedef std::vector<CSession*> SessionList;
    typedef SessionList::iterator ListIt;

    CSession *OnSessionCreate(int fd);
    void OnSessionFinished(CSession *pSession);

    UserAttr m_selfAttr;
    int m_hListenFD;
    int m_Port;
    volatile bool m_bStart;
    SessionList m_Sessions;
    SimpleListener m_Listener;
    SInputReceiver m_STDIN;
};

#endif // SIMPLESERVER_H
