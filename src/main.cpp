#include <iostream>

#include <cstdlib>

#include "simplechat.h"


int main(int argc, char **argv)
{
	SimpleChat *app = new ChatHost(argv[1], atoi(argv[2]));
	app->Init();
	app->Start();
	return 0;
}