#ifndef SIMPLELOGIMPL_H
#define SIMPLELOGIMPL_H


#include <fstream>
#include <cassert>

class logImpl
{
public:
    virtual void WriteToLog(const char *log, int size) = 0;

public:
    logImpl(){}
    virtual ~logImpl(){}
};

class logImpl_CPPFstream : public logImpl
{
public:
    logImpl_CPPFstream(const char *logFileName)
    {
        m_LogFile.open(logFileName, std::ios::app | std::ios::out);
    }
    ~logImpl_CPPFstream()
    {
        if(m_LogFile.is_open())
        {
            m_LogFile.close();
        }
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

#endif