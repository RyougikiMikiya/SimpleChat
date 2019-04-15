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
    stream << logLevel[lvl] << " " << time << " " << filename << " " << lineno << log << std::endl;
    pLogImpl->WriteToLog(stream.str().c_str(), stream.str().size());
}