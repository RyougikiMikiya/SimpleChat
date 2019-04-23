#ifndef SIMPLESERVER_H
#define SIMPLESERVER_H

#include <string>
#include <map>
#include <vector>
#include <deque>
#include <pthread.h>
#include <semaphore.h>

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
        struct SessionReport
        {
           void (SimpleServer::*pReport)(CSession *pSession);
           CSession *pReporter;
        };

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

    int Init(int port);
    int Uninit();

    int Start();
    int Stop();

    int RunServerLoop();

public:
    void OnReceive();

private:
    void PushToAll(const SimpleMsgHdr *pMsg);
    //UI

    //function for login
    uint64_t LoginAuthentication(const AuthInfo &info);
    uint64_t CheckNameExisted(const std::string &name) const;

    //function for report
    void ReportMsgIn(CSession::SessionReport & report);

private:
    typedef std::vector<CSession*> SessionList;
    typedef SessionList::iterator ListIt;

    typedef std::vector<ServerText> RecordList;
    typedef RecordList::iterator RecordIt;

    typedef std::deque<CSession::SessionReport> ReportList;
    CSession *OnSessionCreate(int fd);

    //session report
    void OnSessionFinished(CSession *pSession);

    int m_hListenFD;
    int m_Port;
    volatile bool m_bStart;
    SimpleListener m_Listener;
    sem_t m_ReportSem;

    SessionList         m_Sessions;
    RecordList          m_Records;
    ReportList          m_Reports;
    UserList            m_Users;
    
    pthread_cond_t m_ReportCond;
    pthread_mutex_t m_ReportMutex;


    //function for log
private:
    void PrintAllUser();
};

#endif // SIMPLESERVER_H
