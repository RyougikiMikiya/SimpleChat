#include "SimpleLogImpl.h"

logImpl *SimpleLog::pLogImpl = nullptr;
SimpleLog::LOGLEVEL SimpleLog::slevel = SimpleLog::LOGINFO;
std::string SimpleLog::sLogLocation = "/home/wayne/log/";

void SimpleLog::WriteLog(LOGLEVEL lvl, time_t time, const char *filename, int lineno, const char *log, ...)
{
    static const char *logLevel[] ={
    "LOGDEBUG",
    "LOGINFO",
    "LOGWARN",
    "LOGERROR"
    };

    assert(pLogImpl);
    if(lvl < slevel)
    {
        return;
    }

    char buf[BUFSIZ];
    va_list ap;
    va_start(ap, log);
    vsprintf(buf, log, ap);

    std::stringstream stream;
    stream << logLevel[lvl] << " " << time << " " << filename << " " << lineno << " " << buf << std::endl;
    pLogImpl->WriteToLog(stream.str().c_str(), stream.str().size());
}