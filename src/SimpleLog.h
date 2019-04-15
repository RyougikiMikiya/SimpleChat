#ifndef SIMPLELOG_H
#define SIMPLELOG_H

#include <ctime>
#include <cassert>

#include "SimpleLogImpl.h"

enum LOGLEVEL
{
    LOGDEBUG,
    LOGINFO,
    LOGWARN,
    LOGERR
};

class SimpleLog
{
    public:
    static int LogStart(const char *logFileName)
    {
        assert(logFileName);
        if(pLogImpl != nullptr)
        {
            return 0;
        }
        pLogImpl = new logImpl_CPPFstream(logFileName);
        if(pLogImpl == nullptr)
            assert(false);
    }
    static void SetLogLevel(LOGLEVEL lvl = LOGINFO){ slevel = lvl;}
    static int LogStop()
    {
        if(pLogImpl)
        {
            delete pLogImpl;
        }
    }
    static void WriteLog(LOGLEVEL lvl, time_t time, const char *filename, int lineno, const char *log);
    private:
    static logImpl *pLogImpl;
    static LOGLEVEL slevel;

    SimpleLog(){}
};

logImpl *SimpleLog::pLogImpl = nullptr;
LOGLEVEL SimpleLog::slevel = LOGINFO;

#include <sstream>

#include "SimpleLog.h"

static const char *logLevel[] ={
    "LOGDEBUG",
    "LOGINFO",
    "LOGWARN",
    "LOGERROR"
};

void SimpleLog::WriteLog(LOGLEVEL lvl, time_t time, const char *filename, int lineno, const char *log)
{
    assert(pLogImpl);
    if(lvl < slevel)
    {
        return;
    }
    std::stringstream stream;
    stream << logLevel[lvl] << " " << time << " " << filename << " " << lineno << " " <<log << std::endl;
    pLogImpl->WriteToLog(stream.str().c_str(), stream.str().size());
}


#endif