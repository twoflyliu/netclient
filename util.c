/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 07 15:11:16
*/
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "util.h"

// 获取scheme
// 返回指向主机的第一个字符
static inline const char *
url_get_scheme(const char *surl, Url *url)
{
    const char *purl;
    int n;
    // 首先提取scheme
    purl = surl;
    if ((purl = strstr(purl, "://")) == NULL) {
        strcpy(url->scheme, "http");
        purl = surl;
    } else {
        n = purl - surl;
        strncpy(url->scheme, surl, n); //必须手动添加'\0'
        url->scheme[n] = '\0';
        purl += 3;
    }
    return purl;
}

// 获取主机
// 返回的指针指向端口号，或者指向路径
static inline const char*
url_get_host(const char *surl_without_scheme, Url *url, int *have_port)
{
    const char *purl = NULL;
    int n;
    if ((purl = strchr(surl_without_scheme, ':')) == NULL) {
        *have_port = 0;
        if ((purl = strchr(surl_without_scheme, '/')) != NULL) {
            n = purl - surl_without_scheme;
            strncpy(url->host, surl_without_scheme, n);
            url->host[n] = '\0';
        } else {
            strcpy(url->host, surl_without_scheme); //是形如www.abc.com, 没有路径
            purl = "/"; // 即使没有路径也添加一个路径
        }
    } else {
        *have_port = 1;
        n = purl - surl_without_scheme;
        strncpy(url->host, surl_without_scheme, n);
        url->host[n] = '\0';
        purl ++; // 指向端口号
    }
    return purl;
}

// 获取端口号
// 返回路径的首地址
static inline const char*
url_get_port(const char *surl_without_sh, Url *url)
{
    const char *purl = NULL;
    url->port = strtol(surl_without_sh, (char**)&purl, 10);
    if (*purl != '\0' && *purl != '/') {
        url->errmsg = "parse url failed!: malformed port";
        url->ok = 0;
    }
    return *purl != '\0' ? purl : "/";
}

void url_parse(const char *surl, Url *url)
{
    int have_port;
    memset(url, 0, sizeof(Url));
    url->port = -1;
    url->ok = 1;
    return_if_fail(surl != NULL && url != NULL);
    surl = url_get_scheme(surl, url);
    surl = url_get_host(surl, url, &have_port);
    if (have_port) {
        surl = url_get_port(surl, url); //可能会出错
    } else {
        url->port = -1; // -1 表示端口号没有设置
    }
    if (!url->ok) return;
    strcpy(url->path, surl); // 设置路径
}

#ifdef TEST_UTIL

#ifdef NDEBUG
#undef NDEBUG
#endif

int main(void)
{
    Url url;

    url_parse("www.baidu.com", &url);
    assert(0 == strcmp("http", url.scheme));
    assert(0 == strcmp("www.baidu.com", url.host));
    assert(-1 == url.port);
    assert(0 == strcmp("/", url.path));
    assert(url.ok);

    url_parse("www.baidu.com:80", &url);
    assert(0 == strcmp("http", url.scheme));
    assert(0 == strcmp("www.baidu.com", url.host));
    assert(80 == url.port);
    assert(0 == strcmp("/", url.path));
    assert(url.ok);

    url_parse("https://www.baidu.com:8080/index.html", &url);
    assert(0 == strcmp("https", url.scheme));
    assert(0 == strcmp("www.baidu.com", url.host));
    assert(8080 == url.port);
    assert(0 == strcmp("/index.html", url.path));
    assert(url.ok);

    url_parse("https://www.baidu.com:808k0/index.html", &url);
    assert(!url.ok);

    url_parse("https://www.baidu.com:8080", &url);
    assert(url.ok);

    url_parse("https://www.baidu.com:8080/", &url);
    assert(url.ok);

    return 0;
}

#endif
