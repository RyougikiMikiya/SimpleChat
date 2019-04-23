#include <iostream>
#include <string>
#include <cassert>

#include "SimpleServer.h"
#include "Log/SimpleLogImpl.h"

int main(int argc, char**argv)
{
    logImpl_LinuxLocal *pLogger = new logImpl_LinuxLocal;
    SimpleLog::LogSet(pLogger);
    SimpleLog::LogStart();
    if(argc != 2)
    {
        DLOGERROR("Use \"Server port\" ");
        return -1;
    }
}