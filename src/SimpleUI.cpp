#include <signal.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <cassert>
#include <pthread.h>

#include "SimpleUI.h"
#include "Log/SimpleLog.h"

SimpleUI::~SimpleUI()
{
    assert(m_pOutWindow == nullptr);
    assert(m_pInWindow == nullptr);
}

int SimpleUI::Run()
{
    if(m_bRun)
        return 0;
    if(m_bInit)
        return 0;
    m_bRun = true;

    int ret = -1;

    WINDOW *pWin = initscr();
    assert(pWin == stdscr);
    ret = echo();
    if(ret < 0)
    {
        //error
        goto ERR_NCURESE;
    }

    m_pInWindow = newwin(1, COLS, LINES - 1, 0);
    if(!m_pInWindow)
    {
        //error
        goto ERR_NCURESE;
    } 

    m_pOutWindow = newwin(LINES / 2, COLS, 0, 0);
    if(!m_pOutWindow)
    {
        //error
        goto ERR_NCURESE;
    } 

    m_putMaxRow = getmaxy(m_pOutWindow);

    ret = pthread_create(&m_tReceive, NULL, ReceiveThread, this);
    if(ret < 0)
    {
        //error
        goto ERR_NCURESE;
    }
    m_bInit = true;

    return 0;
ERR_NCURESE:
    endwin();
    return -1;
}

int SimpleUI::Stop()
{
    if(!m_bRun)
        return 0;
    m_bRun = false;
    int ret = pthread_join(m_tReceive, NULL);
    if(ret < 0)
    {
        //error
        ret = -1;
    }
    delwin(m_pInWindow);
    delwin(m_pOutWindow);
    m_pInWindow = nullptr;
    m_pOutWindow = nullptr;
    m_inRow = 0;
    m_putRow = 0;
    m_putMaxRow = 0;
    endwin();
    return ret;
}

int SimpleUI::PrintToScreen(const char *str)
{
    assert(m_bInit);
    if( m_putRow  > (m_putMaxRow/2) )
    {
        werase(m_pOutWindow);
        m_putRow = 0;
    }
    mvwprintw(m_pOutWindow, m_putRow, 0, str);
    DLOGDEBUG("out put %s", str);
    m_putRow++;
    wrefresh(m_pOutWindow);
    return 0;
}

void *SimpleUI::ReceiveThread(void * args)
{
    SimpleUI *pUI = (SimpleUI *)args;
    WINDOW *pInWindow = pUI->m_pInWindow;
    assert(pInWindow);
    char buf[1024];
    while(pUI->m_bRun)
    {
        bzero(buf, 1024);
        mvwgetstr(pInWindow, 0, 0, buf);
        DLOGDEBUG("user input %s", buf);
        wclrtoeol(pInWindow);
        wrefresh(pInWindow);
        struct SimpleClient::UIevent event{SimpleClient::UI_USER_INPUT,  std::string(buf)};
        pUI->m_pClient->putUIevent(event);
    }
    return NULL;
}

//raw : 终端输入不产生信号
//cbreak : 可以捕捉信号