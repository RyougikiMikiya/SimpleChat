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

    static bool SCSort(const SimpleChat *rhs, const SimpleChat *lhs)
    {
        return rhs->m_SelfName < lhs->m_SelfName;
    }

protected:
    virtual void PostMessage() = 0;
    virtual void RecvMessage() = 0;

protected:
    std::string m_SelfName;

    int m_Port;
};

class ChatHost : public SimpleChat
{
public:
    ChatHost(const char *name, int port);

    //overrides
    void Init();
    void Start();


private:
    void PostMessage();
    void RecvMessage();

private:
    int m_ListenFd;
    std::set<SimpleChat*, decltype(SCSort)*> m_ChatUsers;
};

class ChatGuest : public SimpleChat
{
public:
    ChatGuest();
};


#endif // SIMPLECHAT_H
