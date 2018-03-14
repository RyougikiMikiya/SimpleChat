#ifndef __SIMPLE_UTILITY_H__
#define __SIMPLE_UTILITY_H__

struct UserAttr
{
    std::string UesrName;
    //some ohter attrs
};

int readn(int fd, char *pBuf, int n);

int writen(int fd, const char *pBuf, int n);


#endif // __SIMPLE_UTILITY_H__
