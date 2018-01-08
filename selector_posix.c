/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 06 10:01:22
*/
#include "util.h"
#include "selector.h"

#include <sys/select.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

struct _Selector {
    fd_set rset, wset; //!< 保存用户设置
    fd_set rrs, rws;  //!< 接收使用select获取到的结果
    int rmaxfd;
    int wmaxfd;
};

Selector* selector_create()
{
    Selector *thiz = (Selector*)malloc(sizeof(Selector));
    if (thiz != NULL) {
        thiz->rmaxfd = -1;
        thiz->wmaxfd = -1;
        FD_ZERO(&thiz->rset);
        FD_ZERO(&thiz->wset);
        FD_ZERO(&thiz->rrs);
        FD_ZERO(&thiz->rws);
    }
    return thiz;
}

void selector_destroy(Selector *thiz)
{
    return_if_fail(thiz != NULL);
    free(thiz);
}

void selector_add_read(Selector *thiz, int fd)
{
    return_if_fail(thiz != NULL && fd >= 0);
    FD_SET(fd, &thiz->rset);
    if (thiz->rmaxfd < fd) thiz->rmaxfd = fd;
}

void selector_add_write(Selector *thiz, int fd)
{
    return_if_fail(thiz != NULL && fd >= 0);
    FD_SET(fd, &thiz->wset);
    if (thiz->wmaxfd < fd) thiz->wmaxfd = fd;
}

void selector_rm_read(Selector *thiz, int fd)
{
    return_if_fail(thiz != NULL && fd >= 0);
    FD_CLR(fd, &thiz->rset);
    if (fd == thiz->rmaxfd) thiz->rmaxfd--;
}

void selector_rm_write(Selector *thiz, int fd)
{
    return_if_fail(thiz != NULL && fd >= 0);
    FD_CLR(fd, &thiz->wset);
    if (fd == thiz->wmaxfd) thiz->wmaxfd--;
}

int selector_can_read(Selector *thiz, int fd)
{
    return_value_if_fail(thiz != NULL && fd >= 0, 0);
    return FD_ISSET(fd, &thiz->rrs);
}

int selector_can_write(Selector *thiz, int fd)
{
    return_value_if_fail(thiz != NULL && fd >= 0, 0);
    return FD_ISSET(fd, &thiz->rws);
}

int selector_max_fd(Selector *thiz)
{
    return_value_if_fail(thiz != NULL, 0);
    return FD_SETSIZE - 3;
}

void selector_select(Selector *thiz, int millsec)
{
    struct timeval tval;
    int maxfd;
    return_if_fail(thiz != NULL);

    tval.tv_sec = 0;
    tval.tv_usec = 1000 *millsec;

    thiz->rrs = thiz->rset;
    thiz->rws = thiz->wset;

    maxfd = __max(thiz->rmaxfd, thiz->wmaxfd);

redo:
    if (select(maxfd + 1, &thiz->rrs, &thiz->rws, NULL, millsec < 0 ? NULL : &tval) < 0) {
        if (EINTR == errno) goto redo;
        fprintf(stderr, "select error: %s\n", strerror(errno));
        assert(0);
        exit(1);
    }
}
