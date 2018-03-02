#ifndef SIMPLECHAT_H
#define SIMPLECHAT_H

#include <set>
#include <string>


class SimpleChat
{
public:
    SimpleChat(const char *name, int port);
    virtual ~SimpleChat();
    virtual void Init() = 0;
    virtual void Start() = 0;

protected:
    virtual void PostMessage() = 0;
    virtual void RecvMessage() = 0;

protected:
    std::string m_selfName;

    int m_Port;
};

class ChatHost : public SimpleChat
{
public:
    ChatHost();
};

class ChatGuest : public SimpleChat
{
public:
    ChatGuest();
};


#endif // SIMPLECHAT_H
