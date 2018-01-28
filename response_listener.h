#ifndef RESPONSE_EVENT_LISTENER_H_H
#define RESPONSE_EVENT_LISTENER_H_H

#include "event_listener.h"
#include "response.h"

/*!
 * ResponseListener 提供机制来处理各种不同类型的响应
 */

typedef void (*ResponseHandler)(void *thiz, Response *resp);

EventListener* response_listener_create();

// 注意下面的两个函数只有response_listener_create返回的EventListener指针才有效，
// 其他的响应监听器不能使用下面的两个函数，否则会造成程序奔溃的
void response_listener_add_handler(EventListener *thiz, int protocol, void *data, ResponseHandler resp);
void response_listener_remove_handler(EventListener *thiz, int protocol);

#endif //RESPONSE_EVENT_LISTENER_H_H

