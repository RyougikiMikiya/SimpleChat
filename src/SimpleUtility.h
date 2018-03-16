#ifndef __SIMPLE_UTILITY_H__
#define __SIMPLE_UTILITY_H__

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <time.h>
#include <map>

//这两个类暂时放这里
struct UserAttr
{
    uint64_t UID;
    std::string UesrName;
    bool bOnline;
    //some ohter attrs
};

struct AuthInfo
{
    std::string UserName;
    //....some other
};

typedef std::map<uint64_t, UserAttr> UserList;
typedef UserList::iterator UserIt;


int readn(int fd, void *pBuf, int n);

int writen(int fd, const void *pBuf, int n);

inline void DisplayTime(const time_t *stamp, char *pRecvBuf)
{
    struct tm *t;
    t = localtime(stamp);
    sprintf(pRecvBuf, "<%d>:<%d>:<%d>", t->tm_hour, t->tm_min, t->tm_sec);
}

template<typename T>
inline char *PutInStream(void *pDest, T &obj)
{
    assert(pDest);
    char *pTmp = reinterpret_cast<char*>(pDest);
    memcpy(pTmp, &obj, sizeof(obj));
    return pTmp+sizeof(obj);
}

inline char *PutStringInStream(void *pDest, const char *pSrc, int len)
{
    assert(pDest && pSrc);
    char *pTmp = reinterpret_cast<char*>(pDest);
    pTmp = PutInStream(pTmp, len);
    strncpy(pTmp, pSrc, len);
    return pTmp+len;
}

template<typename T>
inline const char *GetInStream(const void *pStream, T *pObj)
{
    assert(pStream && pObj);
    const char *pTmp = reinterpret_cast<const char*>(pStream);
    memcpy(pObj, pStream, sizeof(T));
    return pTmp + sizeof(T);
}

inline const char *GetStringInStream(const void *pStream, char *pObj, int *pLen)
{
    assert(pStream && pObj);
    const char *pTmp = reinterpret_cast<const char*>(pStream);
    pTmp = GetInStream(pStream, pLen);
    assert(*pLen != 0);
    strncpy(pObj, pTmp, *pLen);
    return pTmp + *pLen;
}

#endif // __SIMPLE_UTILITY_H__
