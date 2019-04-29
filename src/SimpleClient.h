#ifndef SIMPLECLIENT_H
#define SIMPLECLIENT_H


/*
Client 业务逻辑:
1.启动UI线程
2.UI告诉用户输入用户名
3.初始化网络模块线程,如果出错则返回网络错误
4.UI返回登陆结果
    ui也需要等待底层消息指示
5.Client分别处理来自UI的输入和来自于服务器的回应
    5.1 UI输入封包之后通过发送到服务器
    5.2 拆解服务器的消息,如果是聊天文本则格式化后告诉UI输出
*/
#include <pthread.h>
#include <netinet/in.h>
#include <deque>
#include <string>
#include <any>

#include "SimpleListener.h"
#include "SimpleMessage.h"
#include "SimpleUtility.h"
#include "SimpleUI.h"

class SimpleUI;


class SimpleClient : public IReceiver
{
public:
    SimpleClient();
    ~SimpleClient(){}

    int Init(const char *pName, const char *pIP, int port);
    int Run();
    int Stop();
//overrides
    void OnReceive();

public:
    enum UIeventType
    {
        UI_USER_INPUT
    };

    //for ui report a event, ui choose a event type and result.
    struct UIevent
    {
        UIeventType eventType;
        std::any arg;
    };

    void putUIevent(UIevent & event);

private:
    int ConnectHost();
    int Login();

    std::string FormatClientText(const ServerText &text) const;
    void PrintMsgToScreen(const SimpleMsgHdr *pMsg);

    const SimpleMsgHdr *Receive();
    int HandleMsg(const SimpleMsgHdr *pMsg);
    
    int Send(SimpleMsgHdr *pMsg)
    {
        return SendMessage(m_hSocket, pMsg);
    }


private:
    SimpleUI *m_pUI;
    std::deque<UIevent> m_EventQueue;
    pthread_mutex_t m_queMutex;
    pthread_cond_t m_Cond;

    UserAttr m_SelfAttr;
    int m_hSocket;
    int m_Port;
    std::string m_strIP;
    in_addr_t m_HostIP;
    SimpleListener m_Listener;
    volatile bool m_bWork;
    //和服务器那边保持一致
    UserList m_Users;

    char m_SendBuf[BUF_MAX_LEN];
    char m_RecvBuf[BUF_MAX_LEN];
};

#endif // SIMPLECLIENT_H
