#ifndef SIMPLESERVER_H
#define SIMPLESERVER_H

#include <string>
#include <vector>

#include "SimpleMessage.h"
#include "SimpleListener.h"

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
        int SessionPost(SimpleMsgHdr *pMsg)
        {
            assert(this);
            PostMessage(m_hFD, pMsg);
        }
        const std::string &GetUserName()const {return m_Attr.UesrName;}

    private:
        struct AuthInfo
        {
            std::string UserName;
        };

        //function self
        SimpleMsgHdr *SessionRecv();
        int HandleMessage(SimpleMsgHdr *pMsg);
        bool LoginAuthentication(AuthInfo &info);



        SimpleServer *m_pServer;
        int m_hFD;
        UserAttr m_Attr;

        char m_SendBuf[2048];
        char m_RecvBuf[2048];
    };

    int Init(const char *pName, int port);
    int Uninit();

    int Start();
    int Stop();

public:
    void OnReceive();

    //fuction for session
protected:
    void PushToAll(SimpleMsgHdr *pMsg);
    bool CheckNameExisted(const std::string &name);
    void DisplayMsg(SimpleMsgHdr *pMsg);

private:
    typedef std::vector<CSession*> SessionList;
    typedef SessionList::iterator ListIt;

    CSession *OnSessionCreate(int fd);
    void OnSessionFinished(CSession *pSession);

    UserAttr m_selfAttr;
    int m_hListenFD;
    int m_Port;
    bool m_bStart;
    SessionList m_Sessions;
    SimpleListener m_Listener;
};

#endif // SIMPLESERVER_H
