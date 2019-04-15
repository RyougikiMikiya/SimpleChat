#include "SimpleLog.h"
#include <ctime>


int main(int argc, char **argv)
{
    SimpleLog::LogStart("mytest.log");
    SimpleLog::WriteLog(LOGERR, time(NULL), __FILE__, __LINE__, "sdfsdfsfsadfasf");
    SimpleLog::LogStop();
    return 0;
}
