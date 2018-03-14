#include <iostream>
#include <string>
#include <cstdlib>
#include <cassert>

#include "SimpleServer.h"


int main(int argc, char **argv)
{
    if(argc != 3 && argc != 4)
    {
        std::cout << "Please enter right params" << std::endl;
        for(int i = 0; i < argc ; i++)
        {
            std::cout << argv[i] << std::endl;
        }
        return -1;
    }

    int port;

    if(argc == 3)
    {
        port = atoi(argv[2]);
        assert(port > 0);
        SimpleServer host;
        host.Init(argv[1], port);
        host.Start();
        host.Stop();
        host.Uninit();
    }

    return 0;
}
