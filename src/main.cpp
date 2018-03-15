#include <iostream>
#include <string>
#include <cstdlib>
#include <cassert>

#include "SimpleClient.h"
#include "SimpleServer.h"


int main(int argc, char **argv)
{
    assert(argc >= 1);
    assert(argv);
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
        if(port < 0)
        {
            return -1;
        }
        SimpleServer host;
        host.Init(argv[1], port);
        host.Start();
        host.Stop();
        host.Uninit();
    }
    else
    {
        port = atoi(argv[3]);
        if(port < 0)
        {
            return -1;
        }
        SimpleClient client;
        client.Init(argv[1], argv[2], port);
        client.Start();
    }

    return 0;
}
