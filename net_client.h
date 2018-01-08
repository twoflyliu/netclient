#ifndef NET_CLIENT_H_H
#define NET_CLIENT_H_H

#include "protocol_client.h"
#include "event_listener.h"
#include "protocol.h"
#include "request.h"

/*!
 * \brief NetClient 是一个ProtcolClient的管理类，和EventListener的管理类
 */
struct _NetClient;
typedef struct _NetClient NetClient;

/*!
 *  \brief net_client_create 工厂函数，用来创建NetClient 
 */
NetClient* net_client_create(void);

/*!
 *  \brief net_client_destroy 析构函数，用来销毁NetClient 
 */
void net_client_destroy(NetClient *thiz);

/*!
 *  \brief net_client_register_protocol 注册新的协议客户端
 *  
 *  使用它可以形成一种插件方式来管理这种协议的客户端，即插即用
 */
void net_client_register_protocol(NetClient *thiz, Protocol protocol,
        ProtocolClientCreateFunc create, void *arg);

/*!
 *  \brief net_client_unregister_protocol 取消某种协议的客户端注册
 */
void net_client_unregister_protocol(NetClient *thiz, Protocol protocol);

/*!
 *  \brief net_client_register_event_listener 注册某种协议的事件监听器
 */
void net_client_add_event_listener(NetClient *thiz, Protocol protocol, EventListener *evt_listener);

/*!
 *  \brief net_client_remove_event_listener 移除指定事件监听器
 */
void net_client_remove_event_listener(NetClient *thiz, Protocol protocol, EventListener *evt_listener);

/*!
 *  \brief net_client_push_request 添加请求任务
 */
void net_client_push_request(NetClient *thiz, Request *request);

/*!
 *  \brief net_client_do_all 等待所有请求处理结束
 *  
 *  在调用此函数之前要先注册好对应的时间监听器方便监控任务进度
 */
void net_client_do_all(NetClient *thiz);

/*!
 *  \brief net_client_set_max_concurrency 设置最大并发数
 */
void net_client_set_max_concurrency(NetClient *thiz, int max_concurrency);

/*!
 *  \brief net_client_set_max_handle_rate 设置网络接收速度
 */
void net_client_set_max_receive_rate(NetClient *thiz, int bytes_per_second);

/*!
 *  \brief net_client_set_timeout 设置超时重连的时间，单位是秒
 */
void net_client_set_timeout(NetClient *thiz, int sec);

/*!
 *  \brief net_client_set_retry_count 设置重连次数
 */
void net_client_set_retry_count(NetClient *thiz, int count);

#endif //NET_CLIENT_H_H
