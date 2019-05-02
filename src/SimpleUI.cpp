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
    assert(!m_bInit);
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

    //输出窗口 大小为屏幕的一半
    m_pOutWindow = newwin(LINES / 2, COLS, 0, 0);
    if(!m_pOutWindow)
    {
        //error
        goto ERR_NCURESE;
    } 

    m_putMaxRow = getmaxy(m_pOutWindow);

    ret = pthread_mutex_init(&m_outputMutex, NULL);
    ret = pthread_create(&m_tReceive, NULL, ReceiveThread, this);
    if(ret < 0)
    {
        //error
        goto ERR_NCURESE;
    }
    m_bInit = true;

    return 0;
ERR_NCURESE:
    m_bRun = false;
    m_bInit = false;
    endwin();
    return -1;
}

int SimpleUI::WaitUIStop()
{
    assert(!m_bRun);
    int ret = pthread_join(m_tReceive, NULL);
    return ret;
}

int SimpleUI::Stop()
{
    DLOGDEBUG("Ready to stop UI");
    if(!m_bRun)
        return 0;
    m_bRun = false;
    int ret = delwin(m_pInWindow);
    ret = delwin(m_pOutWindow);
    ret = endwin();
    m_pInWindow = nullptr;
    m_pOutWindow = nullptr;
    m_inRow = 0;
    m_putRow = 0;
    m_putMaxRow = 0;
    m_bInit = false;
    DLOGDEBUG("Stop UI success");
    return ret;
}

int SimpleUI::PrintToScreen(const char *str)
{
    assert(m_bInit);
    pthread_mutex_lock(&m_outputMutex);
    if( m_putRow  > (m_putMaxRow/2) )
    {
        werase(m_pOutWindow);
        m_putRow = 0;
    }
    mvwprintw(m_pOutWindow, m_putRow, 0, str);
    DLOGDEBUG("out put %s", str);
    m_putRow++;
    wrefresh(m_pOutWindow);
    pthread_mutex_unlock(&m_outputMutex);
    return 0;
}

void *SimpleUI::ReceiveThread(void * args)
{
    assert(args);
    SimpleUI *pUI = (SimpleUI *)args;
    WINDOW *pInWindow = pUI->m_pInWindow;
    assert(pInWindow);
    char buf[1024];
    while(pUI->m_bRun)
    {
        auto status = pUI->m_pClient->GetCliStatus();
        if(status == SimpleClient::CLIENTSTATUS_NEED_LOGIN)
        {
            wclear(pUI->m_pInWindow);
            wclear(pUI->m_pOutWindow);
            pUI->m_putRow = 0;
            mvwprintw(pUI->m_pOutWindow, 0, 0, "Please enter name:");
            pUI->m_putRow++;
            wrefresh(pUI->m_pOutWindow);
            mvwgetstr(pUI->m_pInWindow, 0, 0, buf);
            wclrtoeol(pUI->m_pInWindow);
            wrefresh(pUI->m_pInWindow);
            struct SimpleClient::UIevent event{SimpleClient::UI_USER_LOGIN, std::string(buf)};
            pUI->m_pClient->putUIevent(event);
            pUI->m_pClient->WaitLoginFinish();
            if(  pUI->m_pClient->GetCliStatus() != SimpleClient::CLIENTSTATUS_LOGIN_SUCCESS )
                continue;
        }
        bzero(buf, 1024);
        int ret = mvwgetstr(pInWindow, 0, 0, buf);
        if(ret < 0)
        {
            DLOGDEBUG("input get err!");
            break;
        }
        DLOGDEBUG("user input %d: %s", ret, buf);
        if( (strlen(buf) == 4 ) && !strncmp(buf, "quit", 4) )
        {
            pUI->Stop();
            struct SimpleClient::UIevent event{SimpleClient::UI_USER_QUIT, 0};
            DLOGDEBUG("send quit msg to client");
            pUI->m_pClient->putUIevent(event);
        }
        else if( (strlen(buf) == 6 ) && !strncmp(buf, "logout", 6) )
        {
            struct SimpleClient::UIevent event{SimpleClient::UI_USER_LOGOUT, 0};
            DLOGDEBUG("send logout msg to client");
            pUI->m_pClient->putUIevent(event);
            pUI->m_pClient->WaitLogOutFinish();
            assert( pUI->m_pClient->GetCliStatus() == SimpleClient::CLIENTSTATUS_NEED_LOGIN );
        }
        else
        {
            wclrtoeol(pInWindow);
            wrefresh(pInWindow);
            struct SimpleClient::UIevent event{SimpleClient::UI_USER_INPUT,  std::string(buf)};
            DLOGDEBUG("send put msg to client");
            pUI->m_pClient->putUIevent(event);
        }
    }
    DLOGDEBUG("Exit UI thread");
    return NULL;
}

//raw : 终端输入不产生信号
//cbreak : 可以捕捉信号