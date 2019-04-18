#ifndef SIMPLELOGIMPL_H
#define SIMPLELOGIMPL_H


#include <fstream>
#include <cassert>

#include "SimpleLog.h"

class logImpl_CPPFstream : public logImpl
{
public:
    logImpl_CPPFstream()
    {
    }
    ~logImpl_CPPFstream()
    {
        assert(!m_LogFile.is_open());
    }

    void WriteToLog(const char *log, int size)
    {
        assert(m_LogFile.is_open());
        assert(m_LogFile.good());
        m_LogFile << log;
    }
private:
    std::ofstream m_LogFile;
};


#include <unistd.h>
#include <cerrno>
#include <sys/types.h>
#include <fcntl.h>

class logImpl_LinuxLocal : public logImpl
{
public:
    logImpl_LinuxLocal():m_logFD(-1)
    {

    }
    ~logImpl_LinuxLocal()
    {
        assert(m_logFD == -1);
    }

    bool InitLogger(const char *name)
    {
        if(m_logFD >= 0)
            return true;
        m_logFD = open(name, O_RDWR | O_APPEND | O_CREAT, 0644);
        if(m_logFD < 0)
        {
            perror("open log file failed");
            return false;
        }
        return true;
    }

    bool CloseLogger()
    {
        if(m_logFD >= 0)
        {
            int ret = close(m_logFD);
            if(ret < 0)
            {
                perror("close");
                return false;
            }
            m_logFD = -1;
        }
        return true;
    }

    bool NewLogger(const char *name)
    {
        if(m_logFD < 0)
        {
            InitLogger(name);
        }
        int tempFD = m_logFD;
        if( !InitLogger(name))
        {
            return false;
        }
        int ret = close(tempFD);
        if(ret < 0)
        {
            //error handle;
        }
        return true;
        
    }
    
    void WriteToLog(const char *log, int size)
    {
        if(m_logFD < 0)
        {
            ;
        }
        int ret = write(m_logFD, log, size);
        if(ret < 0)
        {
            ;
        }
    }

private:
    int m_logFD;
};

#endif