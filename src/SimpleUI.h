#ifndef SIMPLEUI_H
#define SIMPLEUI_H

#include "ncurses.h"
#include "SimpleClient.h"
#include <pthread.h>

class SimpleClient;

class SimpleUI
{
public:
    SimpleUI(SimpleClient *client): 
        m_bRun(false), m_pOutWindow(NULL), m_pInWindow(NULL), m_pClient(client), 
        m_inRow(0), m_putRow(0), m_putMaxRow(0), m_bInit(false)
    {

    }
    ~SimpleUI();

    int Run();
    int WaitUIStop();
    int PrintToScreen(const char *str);

private:
    static void *ReceiveThread(void *);

    void LoginUI();
    int Stop();
    
    volatile bool m_bRun;
    bool m_bInit;
    WINDOW *m_pOutWindow;
    WINDOW *m_pInWindow;
    int m_inRow;//输入光标行号
    int m_putRow;//输出光标行号
    int m_putMaxRow;//输出光标最大行号

    SimpleClient *m_pClient;
    pthread_t m_tReceive;
    pthread_mutex_t m_outputMutex;
};

#endif