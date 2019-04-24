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
    DLOGINFO("ready Start");
    if (argc != 4)
    {
        cout << "Use \"Client name ip port\" " << endl;
        return -1;
    }

    cout << atoi(argv[3]) << endl;
    SimpleClient cli;
    int ret = cli.Init(argv[1], argv[2], atoi(argv[3]));
    ret = cli.Start();
    sleep(300);
    ret = cli.Stop();

    return 0;
}