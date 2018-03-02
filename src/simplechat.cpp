#include "simplechat.h"

SimpleChat::SimpleChat(const char *name, int port): m_SelfName(name), m_Port(port)
{

}

ChatHost::ChatHost(const char *name, int port) : SimpleChat(name, port), m_ChatUsers(SCSort)
{

}

void ChatHost::Init()
{

}

void ChatHost::Start()
{

}

void ChatHost::PostMessage()
{

}

void ChatHost::RecvMessage()
{

}

