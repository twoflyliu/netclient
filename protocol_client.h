#ifndef PROTOCOL_CLIENT_H_H
#define PROTOCOL_CLIENT_H_H

#include <stddef.h>
#include "util.h"

struct _ProtocolClient;
typedef struct _ProtocolClient ProtocolClient;

/*!
 *  \brief ClientStat 描述各种ProtoclClient的处理状态 
 */
typedef enum _ClientStat {
    CLIENT_STAT_FINAL = 0,     //!< 表示结束处理了, 其他状态表示没有结束
} ClientStat;

#include "notifier.h"
#include "request.h"

/*!
 *  \brief ProtocolClient 一个表示网络客户端的接口
 *  
 *  这个接口主要是实现具体的网络信息，只是实现客户端部分。
 *  
 *  比如： 如果这个接口的实现者是HttpClient，那么他就应该实现http请求头的创建和发送
 *  如果需要发送请求体数据的话，也要负责发送，并且接受一些响应数据 
 *  
 *  Notifier 主要是用来通知观察者自身的状态变化，比如操作成功，或者操作失败，想输出
 *  某些日志，都可能使用它来实现。
 */
struct _ProtocolClient {
    /*!
     *  \brief handle 进行协议的相关任务，因为本系统使用的网络套接字都是异步的所以这个操作是非阻塞
     *  操作，可能要操作若干次才能够完成整个任务。 
     *  
     *  一般实现就是内部是会使用一种状态机实现用来维护当前处在什么进度上
     *  
     *  \param fd [out] 用来接收内部的文件描述符
     *  \param want_read [out] 表明内部想要检测套接字是否可读
     *  \param want_write [out] 表明内部想要检测套接字是否可写
     */
    int (*handle)(ProtocolClient *thiz, int *fd, int *want_read, int *want_write);

    /*!
     *  \brief get_sockfd 获取内部的sockfd, 调用此函数前必须要先执行过handle
     */
    int (*get_sockfd)(ProtocolClient *thiz);

    /*!
     *  \brief set_timeout 用来设置内部超时时间
     */
    void (*set_timeout)(ProtocolClient *thiz, int sec);

    /*!
     *  \brief set_retry_count 用来设置当失败的时候，进行重新尝试次数 
     */
    void (*set_retry_count)(ProtocolClient *thiz, int retry_count);

    /*!
     *  \brief get_resp_incre_len 获取内部响应增量长度，方便管理者用来统计下载速度 
     */
    int (*get_resp_incre_len)(ProtocolClient *thiz);

    /*!
     *  \brief on_timed 提供这种函数方便用来检测内部自身是否超时终止了 
     */
    ClientStat (*on_idle)(ProtocolClient *thiz);

    /*
     *  \brief destroy 析构函数
     */
    void (*destroy)(ProtocolClient *thiz);

    char priv[0]; //!< 其他私有数据
};

static inline void protocol_client_destroy(ProtocolClient *thiz)
{
    return_if_fail(thiz != NULL && thiz->destroy != NULL);
    thiz->destroy(thiz);
}

//!< 一个ProtocolClient工厂函数原型, 因为不知道具体协议实现，不清楚要不要参数，这儿预留一个，方便以后扩展
typedef ProtocolClient* (*ProtocolClientCreateFunc)(Notifier *notifier, void *arg, Request *request); 

#endif //PROTOCOL_CLIENT_H_H
