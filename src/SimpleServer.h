#ifndef SIMPLESERVER_H
#define SIMPLESERVER_H

#include <string>
#include <map>
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
            return SendMessage(m_hFD, pMsg);
        }
        uint64_t GetUID() const{return m_UID;}

    private:
        //function self
        const SimpleMsgHdr *Recevie();
        int HandleMessage(const SimpleMsgHdr *pMsg);

        SimpleServer *m_pServer;
        int m_hFD;
        uint64_t m_UID;

        char m_SendBuf[BUF_MAX_LEN];
        char m_RecvBuf[BUF_MAX_LEN];
    };

    int Init(const char *pName, int port);
    int Uninit();

    int Start();
    int Stop();

public:
    void OnReceive();

private:
    void PushToAll(const SimpleMsgHdr *pMsg);

    //UI
    std::string FormatServerText(const ServerText &text);
    void PrintMsgToScreen(const SimpleMsgHdr *pMsg);

    //function for login
    uint64_t LoginAuthentication(const AuthInfo &info);
    uint64_t CheckNameExisted(const std::string &name) const;
    void RegistUser(UserAttr &info);

private:
    class SInputReceiver : public IReceiver
    {
    public:
        SInputReceiver(SimpleServer *pServer) : m_pServer(pServer){}
        void OnReceive();
    private:
        SimpleServer *m_pServer;
        char m_SendBuf[BUF_MAX_LEN];
        std::string m_LineBuf;
    };

private:
    typedef std::vector<CSession*> SessionList;
    typedef SessionList::iterator ListIt;

    typedef std::vector<ServerText> RecordList;
    typedef RecordList::iterator RecordIt;

    CSession *OnSessionCreate(int fd);
    void OnSessionFinished(CSession *pSession);

    UserAttr m_SelfAttr;
    int m_hListenFD;
    int m_Port;
    volatile bool m_bStart;
    SimpleListener m_Listener;
    SInputReceiver m_STDIN;

    SessionList m_Sessions;
    RecordList m_Records;
    UserList m_Users;


    //function for log
private:
    void PrintAllUser();
};

#endif // SIMPLESERVER_H
