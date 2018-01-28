#ifndef UTIL_H_H
#define UTIL_H_H

#include <stdio.h>

#define return_if_fail(cond) \
    if (!(cond)) {\
        fprintf(stderr, "WARNING:%s:%d: %s is FALSE\n", __FILE__, __LINE__, #cond);\
        return;\
    }

#define return_value_if_fail(cond, retval) \
    if (!(cond)) {\
        fprintf(stderr, "WARNING:%s:%d: %s is FALSE\n", __FILE__, __LINE__, #cond);\
        return retval;\
    }

#define __max(a, b) ((a) > (b) ? (a) : (b))
#define __min(a, b) ((a) < (b) ? (a) : (b))

#define UNUSED __attribute__((unused))

/*!
 * \brief 描述一个基本的url
 */
typedef struct _Url {
    char scheme[10]; //!< 具体协议，比如:http, https, ftp...
    char host[50];   //!< 主机名称，比如：www.abc.com,...
    char path[200];  //!< 具体主机路径名，比如:/, /index.jsp...
    //char args[200];
    //char anchor[50];
    int  port;      //!< 保存主机端口号
    int  ok;     //!< 保存解析的状态，1表示解析成功，0表示失败
    const char *errmsg; //!< 当ok为false时候，返回的出错信息
} Url;

/*!
 * \brief url_parse 解析url
 * \param [in] surl 要解析的url字符串
 * \param [out] url 用来保存解析后的结果
 */
void url_parse(const char *surl, Url *url);

/*!
 * \brief _stricmp 忽略大小写比较两个字符串的大小关系
 * 
 * 提供这个函数主要是为了解决msys2上string.h不包含stricmp函数问题
 * 
 * \retval 0=> s1 == s2, -1 => s1 < s2, 1 => s1 > s2
 */
int _stricmp(const char *s1, const char *s2);

#endif //UTIL_H_H
