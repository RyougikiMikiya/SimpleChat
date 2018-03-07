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

    SimpleChat *pApp;
    if(argc == 3)
    {
        int port = atoi(argv[2]);
        assert(port > 0);
        ChatHost(port);
        if(!pApp)
        {
            std::cout << "Failed to allocate memory!" << std::endl;
            return -1;
        }
    }
    else if(argc == 4)
    {
        int port = atoi(argv[3]);
        assert(port > 0);
        pApp = new ChatGuest(port);
        if(!pApp)
        {
            std::cout << "Failed to allocate memory!" << std::endl;
            return -1;
        }
    }

    int ret;

    ret = pApp->Create(argc, argv);
    if(ret < 0)
        return -1;

    pApp->Init();
    if(ret < 0)
        return -1;

    pApp->Run();

    return 0;
}
