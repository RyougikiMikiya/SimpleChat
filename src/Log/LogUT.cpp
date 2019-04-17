#include "SimpleLog.h"
#include "SimpleLogImpl.h"
#include <ctime>


int main(int argc, char **argv)
{
    logImpl_LinuxLocal *pLog = new logImpl_LinuxLocal;
    SimpleLog::LogSet(pLog);
    SimpleLog::LogStart();
    DLOGDEBUG("Debug");
    DLOGINFO("INFO");
    DLOGERROR("ERROR");
    DLOGWARN("WARN");
    SimpleLog::LogStop();
    delete pLog;
    return 0;
}
