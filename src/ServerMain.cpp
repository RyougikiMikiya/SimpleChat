#include <iostream>
#include <string>
#include <cassert>
#include <cstdlib>

#include "SimpleServer.h"
#include "Log/SimpleLogImpl.h"

using namespace std;

int main(int argc, char **argv)
{
    logImpl_LinuxLocal *pLogger = new logImpl_LinuxLocal;
    SimpleLog::LogStart();
    DLOGINFO("ready Start");
    if (argc != 2)
    {
        cout << "Use \"Server port\" " << endl;
        return -1;
    }

    cout << atoi(argv[1]) << endl;
    SimpleServer ser;
    if (ser.Init(atoi(argv[1])) != 0)
    {
        std::cout << "Init failed" << std::endl;
        return -1;
    }
    int ret = ser.Start();
    cout << "Start " << ret << endl;
    ser.RunServerLoop();
    ret = ser.Stop();
    cout << "Stop " << ret << endl;
    ret = ser.Uninit();
    cout << "uninit" << ret << endl;

    return 0;
}