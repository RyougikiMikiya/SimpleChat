all:
	g++ -o Server ServerMain.cpp SimpleListener.cpp SimpleServer.cpp SimpleUtility.cpp SimpleMessage.cpp Log/SimpleLog.cpp -I./Log -pthread -std=c++11

clinet:
	g++ -o Client ClientMain.cpp SimpleListener.cpp SimpleClient.cpp SimpleUtility.cpp SimpleMessage.cpp Log/SimpleLog.cpp -I./Log -pthread -std=c++11