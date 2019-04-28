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

//设置一个线程去跑run函数?
int SimpleUI::Run()
{
    if(m_bRun)
        return 0;
    m_bRun = true;

    int ret = Init();
    if(ret < 0)
    {
        return -1;
    }

    WINDOW *pWin = initscr();
    assert(pWin == stdscr);
    ret = echo();
    if(ret < 0)
    {
        //error
        goto ERR_NCURESE;
    }
    pInWindow = newwin(1, COLS, LINES - 1, 0);
    if(!pInWindow)
    {
        //error
        goto ERR_NCURESE;
    } 

    m_pOutWindow = newwin(LINES / 2, COLS, 0, 0);
    if(!pInWindow)
    {
        //error
        goto ERR_NCURESE;
    } 

    char buf[1024];
    while (m_bRun)
    {
        bzero(buf, 1024);
        mvwgetstr(m_pInWindow, 0, 0, buf);
        clrtoeol();
        wrefresh(m_pInWindow);
        struct SimpleClient::UIevent event{SimpleClient::UI_USER_INPUT,  std::string(buf)};
        m_pClient->putUIevent(event);
    }
    delwin(pInWindow);
    delwin(m_pOutWindow);
    endwin();
    return 0;

ERR_NCURESE:
    endwin();
    return -1;
}

int SimpleUI::Init()
{
    int ret = pthread_mutex_init(&m_curseMutex, NULL);
    //.....
    if(ret < 0)
    {
        DLOGERROR("pthread_mutex_init: %s", strerror(errno));
        return ret;
    }

    return ret;
}

int SimpleUI::PrintToScreen(const char *str)
{
    if( m_putRow  > (m_putMaxRow/2) )
    {
        werase(m_pOutWindow);
        m_putRow = 0;
    }
    mvwprintw(m_pOutWindow, m_putRow, 0, str);
    m_putRow++;
    wrefresh(m_pOutWindow);
    return 0;
}

//raw : 终端输入不产生信号
//cbreak : 可以捕捉信号