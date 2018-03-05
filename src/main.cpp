#include <iostream>
#include <string>
#include <cstdlib>

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

    SimpleChat *app;
    if(argc == 3)
    {
        std::string name(argv[1]);
        int port = atoi(argv[2]);
        app = new ChatHost(name, port);
    }
    app->Init();
    app->Start();
	return 0;
}
