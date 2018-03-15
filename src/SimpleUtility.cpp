#include <unistd.h>
#include <errno.h>

#include "SimpleUtility.h"

int readn(int fd, void *pBuf, int n)
{
    int   nLeft;
    int   nRead;
    char *pTmp;

    pTmp = reinterpret_cast<char*>(pBuf);
    nLeft = n;
    while (nLeft > 0)
    {
        nRead = read(fd, pTmp, nLeft);
        if (nRead < 0)
        {
            if (errno == EINTR)
                nRead = 0;      /* and call read() again */
            else
                return -1;
        }
        else if (nRead == 0)
            break;              /* EOF */
        nLeft -= nRead;
        pTmp   += nRead;
    }
    return (n - nLeft);     /* return >= 0 */
}

int writen(int fd, const void *pBuf, int n)
{
    int     nLeft;
    int     nWritten;
    const char  *pTmp;

    pTmp = reinterpret_cast<const char*>(pBuf);
    nLeft = n;
    while (nLeft > 0)
    {
        nWritten = write(fd, pTmp, nLeft);
        if ( nWritten < 0)
        {
            if (nWritten < 0 && errno == EINTR)
                nWritten = 0;       /* and call write() again */
            else
                return(-1);         /* error */
        }
        nLeft -= nWritten;
        pTmp   += nWritten;
    }
    return n;
}
