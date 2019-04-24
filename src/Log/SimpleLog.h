#ifndef SIMPLELOG_H
#define SIMPLELOG_H

#include <ctime>
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <cstdarg>

class logImpl
{
public:
    virtual void WriteToLog(const char *log, int size) = 0;
    virtual bool InitLogger(const char *fileName) = 0;
    virtual bool CloseLogger() = 0;

public:
    logImpl(){}
    virtual ~logImpl(){}
};

class SimpleLog
{
    public:
    enum LOGLEVEL
    {
        LOGDEBUG,
        LOGINFO,
        LOGWARN,
        LOGERR
    };

    static void LogSet(logImpl *pImpl)
    {
        assert(pImpl);
        pLogImpl = pImpl;
    }
    static void SetLogLevel(LOGLEVEL lvl = LOGINFO){ slevel = lvl;}
    static void SetLogDir(const char * path){ sLogLocation = path;}
    static int LogStart()
    {
        assert(pLogImpl);
        time_t now = time(NULL);
        std::stringstream s;
        s << sLogLocation << now;
        if( !pLogImpl->InitLogger(s.str().c_str()) )
        {
            return -1;
        }
        return 0;
    }
    static int LogStop()
    {
        assert(pLogImpl);
        pLogImpl->CloseLogger();
        
    }
    static void WriteLog(LOGLEVEL lvl, time_t time, const char *filename, int lineno, const char *log, ...);
    private:
    static logImpl *pLogImpl;
    static LOGLEVEL slevel;
    static std::string sLogLocation ;

    SimpleLog(){}
};

#define DLOGDEBUG(log, ...) SimpleLog::WriteLog(SimpleLog::LOGDEBUG, time(NULL), __FILE__, __LINE__, log, ##__VA_ARGS__)
#define DLOGINFO(log, ...) SimpleLog::WriteLog(SimpleLog::LOGINFO, time(NULL), __FILE__, __LINE__, log, ##__VA_ARGS__)
#define DLOGWARN(log, ...) SimpleLog::WriteLog(SimpleLog::LOGWARN, time(NULL), __FILE__, __LINE__, log, ##__VA_ARGS__)
#define DLOGERROR(log, ...) SimpleLog::WriteLog(SimpleLog::LOGERR, time(NULL), __FILE__, __LINE__, log, ##__VA_ARGS__)



#endif