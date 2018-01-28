/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 05 17:48:54
*/
#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "net_client.h"
#include "list.h"
#include "selector.h"

// 一般应用层协议不会超过100的吧，应该吧

typedef struct _ProtocolClientCreator {
    void *arg;
    ProtocolClientCreateFunc creator;
} ProtocolClientCreator;

struct _NetClient {
    Notifier *notifier;
    ProtocolClientCreator creators[MAX_PROTCOL]; //协议号是天然的hash value, 不会有冲突
    List *clients;
    Selector *selector;
    int max_concurrency;
    int max_receive_rate;
};

/*!
 *  \brief net_client_create 工厂函数，用来创建NetClient 
 */
NetClient* net_client_create(void)
{
    NetClient *thiz = (NetClient*)calloc(sizeof(NetClient), 1);
    if (thiz != NULL) {
        thiz->notifier = notifier_create();
        thiz->clients = list_create((DataDestroyFunc)protocol_client_destroy);
        thiz->selector = selector_create();
        thiz->max_concurrency = selector_max_fd(thiz->selector) - 5; // 减5是为了保留5个文件描述以待他用，比如日志。。。
        thiz->max_receive_rate = -1; // 负值表示不限速
    }
    return thiz;
}

/*!
 *  \brief net_client_destroy 析构函数，用来销毁NetClient 
 */
void net_client_destroy(NetClient *thiz)
{
    return_if_fail(thiz != NULL);
    notifier_destroy(thiz->notifier);
    list_destroy(thiz->clients);
    selector_destroy(thiz->selector);
    free(thiz);
}

/*!
 *  \brief net_client_register_protocol 注册新的协议客户端
 *  
 *  使用它可以形成一种插件方式来管理这种协议的客户端，即插即用
 */
void net_client_register_protocol(NetClient *thiz, Protocol protocol,
        ProtocolClientCreateFunc create, void *arg)
{
    return_if_fail(thiz != NULL && protocol != PROTOCOL_ALL && create != NULL);
    thiz->creators[protocol].arg = arg;
    thiz->creators[protocol].creator = create;
}

/*!
 *  \brief net_client_unregister_protocol 取消某种协议的客户端注册
 */
void net_client_unregister_protocol(NetClient *thiz, Protocol protocol)
{
    return_if_fail(thiz != NULL);
    if (PROTOCOL_ALL == protocol) {
        for (int i = 0; i < MAX_PROTCOL; i++) {
            thiz->creators[i].arg = NULL;
            thiz->creators[i].creator = NULL;
        }
    } else {
        thiz->creators[protocol].arg = NULL;
        thiz->creators[protocol].creator = NULL;
    }
}

/*!
 *  \brief net_client_register_event_listener 注册某种协议的事件监听器
 */
void net_client_add_event_listener(NetClient *thiz, Protocol protocol, EventListener *evt_listener)
{
    return_if_fail(thiz != NULL && evt_listener != NULL && thiz->notifier != NULL);
    notifier_add_event_listener(thiz->notifier, protocol, evt_listener);
}

/*!
 *  \brief net_client_remove_event_listener 移除指定事件监听器
 */
void net_client_remove_event_listener(NetClient *thiz, Protocol protocol, EventListener *evt_listener)
{
    return_if_fail(thiz != NULL && evt_listener != NULL && thiz->notifier != NULL);
    notifier_remove_event_listener(thiz->notifier, protocol, evt_listener);
}


/*!
 *  \brief net_client_push_request 添加请求任务
 */
void net_client_push_request(NetClient *thiz, Request *request)
{
    ProtocolClientCreator *creator = NULL;
    return_if_fail(thiz != NULL && request != NULL);
    creator = &thiz->creators[request->protocol];
    return_if_fail(creator->creator != NULL);
    list_push_back(thiz->clients, 
            creator->creator(thiz->notifier, creator->arg, request)); // 增加任务
}

/*!
 *  \brief net_client_do_all 等待所有请求处理结束
 *  
 *  在调用此函数之前要先注册好对应的时间监听器方便监控任务进度
 */
void net_client_do_all(NetClient *thiz)
{
    int i, ofd, fd, ret, want_read, want_write;
    ProtocolClient *client = NULL;
    Iterator *iter = NULL;
    return_if_fail(thiz != NULL);
    if (list_size(thiz->clients) == 0) return;

    /*thiz->max_concurrency = 1; // 方便并发测试*/

    // 首先进行第一轮处理话，主要是为了获取获取文件描述符，并且进行第一次多路复用
    i = 0;
    for (iter = list_begin(thiz->clients); !iter->is_done(iter) 
            && i < thiz->max_concurrency; iter->next(iter), i++) {
        client = (ProtocolClient*)iterator_data(iter);
        client->handle(client, &fd, &want_read, &want_write);
        return_if_fail(fd != -1);
        if (want_read) selector_add_read(thiz->selector, fd);
        if (want_write) selector_add_write(thiz->selector, fd);
    }
    iterator_destroy(iter);

    do {
        selector_select(thiz->selector, 100); // 最多等待100毫秒
        i = 0;
        for (iter = list_begin(thiz->clients); !iter->is_done(iter) 
                && i < thiz->max_concurrency; ) {
            client = (ProtocolClient*)iterator_data(iter);
            ofd = fd = client->get_sockfd(client);
            // fd < 0表明还没有建立新建（是新任务），另外两个条件表明真正可以进度读写操作
            if (fd < 0 || selector_can_read(thiz->selector, fd) || selector_can_write(thiz->selector, fd)) {
                ret = client->handle(client, &fd, &want_read, &want_write);
                if (fd >= 0) { // 如果fd返回-1，表明任务已经结束
                    if (ofd > -1 && ofd != fd) { //说明内部发生了重定向
                        selector_rm_read(thiz->selector, ofd);
                        selector_rm_write(thiz->selector, ofd);
                    }
                    if (want_read) {
                        selector_rm_write(thiz->selector, fd); // 因为不确定之前是否绑定他了，所以使用这种笨方法
                        selector_add_read(thiz->selector, fd);
                    }
                    if (want_write) {
                        selector_rm_read(thiz->selector, fd);
                        selector_add_write(thiz->selector, fd);
                    }
                }
            } else {
                ret = client->on_idle(client, &fd, &want_read, &want_write); // 内部可以进行超时重连
            }

            if (CLIENT_STAT_FINAL == ret) {
                fd = client->get_sockfd(client);
                assert(fd > 0);
                selector_rm_read(thiz->selector, fd);
                selector_rm_write(thiz->selector, fd);
                iterator_erase(iter); // 因为处理完了所以删除任务
                continue;
            }

            iterator_next(iter);
            ++i;
        }
        iterator_destroy(iter);
    } while (list_size(thiz->clients) > 0);
}

/*!
 *  \brief net_client_set_max_concurrency 设置最大并发数
 */
void net_client_set_max_concurrency(NetClient *thiz, int max_concurrency)
{
    return_if_fail(thiz != NULL && max_concurrency > 0);
    if (max_concurrency < thiz->max_concurrency) { // 不能够超过Selector最大支持的文件描述符数
        thiz->max_concurrency = max_concurrency;
    }
}

/*!
 *  \brief net_client_set_max_handle_rate 设置网络接收速度
 */
void net_client_set_max_receive_rate(NetClient *thiz, int bytes_per_second)
{
    return_if_fail(thiz != NULL);
    thiz->max_receive_rate = bytes_per_second;
}

/*!
 *  \brief net_client_set_timeout 设置超时重连的时间，单位是秒
 */
void net_client_set_timeout(NetClient *thiz, int sec)
{
    Iterator *iter = NULL;
    if (list_empty(thiz->clients)) return;
    for (iter = list_begin(thiz->clients); !iterator_is_done(iter); iterator_next(iter)) {
        ProtocolClient *client = (ProtocolClient*)iterator_data(iter);
        if (client->set_timeout) client->set_timeout(client ,sec);
    }
    if (iter != NULL) iterator_destroy(iter);
}

/*!
 *  \brief net_client_set_retry_count 设置重连次数
 */
void net_client_set_retry_count(NetClient *thiz, int count)
{
    Iterator *iter = NULL;
    if (list_empty(thiz->clients)) return;
    for (iter = list_begin(thiz->clients); !iterator_is_done(iter); iterator_next(iter)) {
        ProtocolClient *client = (ProtocolClient*)iterator_data(iter);
        if (client->set_retry_count) client->set_retry_count(client, count);
    }
    if (iter != NULL) iterator_destroy(iter);
}
