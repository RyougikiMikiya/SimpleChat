#include <iostream>
#include <string>
#include <cstdlib>
#include <cassert>

#include "SimpleChat.h"


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
        ChatHost host(port);
        host.Init(argv[1]);
        host.Start();
        host.Stop();
        host.Uninit();
    }
    else if(argc == 4)
    {
        port = atoi(argv[3]);
        assert(port > 0);
        ChatGuest guest(port);
        guest.Init(argv[2], argv[1]);
        guest.Start();
    }

    return 0;
}
