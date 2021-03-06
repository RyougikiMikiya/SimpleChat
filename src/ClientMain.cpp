#include <iostream>
#include <string>
#include <cassert>
#include <cstdlib>

#include "SimpleClient.h"
#include "Log/SimpleLogImpl.h"

using namespace std;

int main(int argc, char **argv)
{
    logImpl_LinuxLocal *pLogger = new logImpl_LinuxLocal;
    SimpleLog::LogSet(pLogger);  
    SimpleLog::LogStart();
    SimpleLog::SetLogLevel(SimpleLog::LOGDEBUG);
    DLOGINFO("ready Start");
    if (argc != 3)
    {
        cout << "Use \"Client ip port\" " << endl;
        return -1;
    }

    cout << atoi(argv[2]) << endl;
    SimpleClient cli;
    int ret = cli.Init(argv[1], atoi(argv[2]));
    if(ret < 0)
    {
        return -1;
    }
    ret = cli.Run();

    return 0;
}