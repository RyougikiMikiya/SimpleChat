#include <iostream>
#include <vector>
#include "ncurses.h"
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <cstring>
#include <deque>

using namespace std;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
sem_t sem;
deque<string> sList;

void *inThread(void *)
{
    WINDOW *pInWindow = newwin(1, COLS, LINES - 1, 0);
    char buf[1024] = "helloworld";
    while (1)
    {
        curs_set(1);
        bzero(buf, 1024);
        mvwgetstr(pInWindow, 0, 0 ,buf);
        pthread_mutex_lock(&mutex);
        sList.push_back(buf);
        werase(pInWindow);
        wrefresh(pInWindow);
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond);
        
    }
    delwin(pInWindow);
}

void *putThread(void *)
{
    WINDOW *pOutWindow = newwin(LINES / 2, COLS, 0, 0);
    int curRow = 0;
    int maxRow = getmaxy(pOutWindow);
    while (1)
    {
        pthread_mutex_lock(&mutex);
        while (sList.empty())
        {
            pthread_cond_wait(&cond, &mutex);
        }
        curs_set(0);
        string temp = sList.front();
        sList.pop_front();
        if (curRow == (maxRow - 1))
        {
            werase(pOutWindow);
            curRow = 0;
        }
        mvwprintw(pOutWindow, curRow, 0, temp.c_str());
        wrefresh(pOutWindow);
        pthread_mutex_unlock(&mutex);
        curRow++;
    }
    delwin(pOutWindow);
}

int main()
{
    initscr();
    sem_init(&sem, 0, 0);

    pthread_t t1, t2;
    pthread_create(&t1, NULL, inThread, NULL);
    pthread_create(&t2, NULL, putThread, NULL);

    sem_destroy(&sem);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    endwin();
    return 0;
}