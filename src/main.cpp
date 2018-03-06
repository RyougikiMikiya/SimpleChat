#include <iostream>
#include <string>
#include <cstdlib>
#include <cassert>

#include "simpleChat.h"


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
        pApp = new ChatHost(port);
        if(!pApp)
        {
            std::cout << "Failed to allocate memory!" << std::endl;
            return -1;
        }
        pApp->Create(argc, argv);
        pApp->Init();
        pApp->Run();
    }
    else if(argc == 4)
    {

    }

    return 0;
}
