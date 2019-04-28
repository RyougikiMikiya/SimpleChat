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
        m_curRow(0), m_putRow(0), m_maxRow(0)
    {

    }
    int Run();
    int PrintToScreen(const char *str);
    ~SimpleUI();

private:

    int Init();

    static void ReceiveThread(SimpleUI *);

    
    bool m_bRun;
    WINDOW *m_pOutWindow;
    WINDOW *m_pInWindow;
    int m_inRow;//输入光标行号
    int m_putRow;//输出光标行号
    int m_putMaxRow;//输出光标最大行号


    SimpleClient *m_pClient;
    pthread_t m_tReceive;
    pthread_mutex_t m_curseMutex;
};

#endif