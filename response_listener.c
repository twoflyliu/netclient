/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 08 16:34:09
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "response.h"
#include "response_listener.h"

#include "protocol.h"

static void response_listener_handle(struct _EventListener *thiz, Event *event);
static void response_listener_destroy(struct _EventListener *thiz);

typedef struct _EventListenerPriv {
    struct {
        void *data;
        ResponseHandler handler;
    } handlers[MAX_PROTCOL]; //数组是天然的hash表
} EventListenerPriv;

// 只是将响应的基本信息给打印出来，对于具体的
// 响应比如http响应，需要根据自己需求来进行操作
EventListener* response_listener_create()
{
    EventListener* thiz = (EventListener*)malloc(sizeof(EventListener) + sizeof(EventListenerPriv));
    if (thiz != NULL) {
        EventListenerPriv *priv = (EventListenerPriv*)thiz->priv;
        thiz->handle = response_listener_handle;
        thiz->destroy = response_listener_destroy;
        memset(priv, 0, sizeof(EventListenerPriv));
    }
    return thiz;
}

static void response_listener_destroy(struct _EventListener *thiz)
{
    free(thiz);
}

// 只是输出response的基本信息，对于确定的协议，要根据你的实际需求来操作
static void response_listener_handle(struct _EventListener *thiz, Event *event)
{
    ResponseEvent *resp_event = NULL;
    Response *resp = NULL;
    EventListenerPriv *priv = (EventListenerPriv*)thiz->priv;

    if (event->event_type != EVENT_TYPE_RESPONSE) return;
    resp_event = (ResponseEvent*)event;
    resp = resp_event->response;

    // 如果没有注册响应处理器的话，则简单打印调用对应的处理器进行处理
    if (priv->handlers[resp->protocol].handler != NULL) {
        priv->handlers[resp->protocol].handler(priv->handlers[resp->protocol].data, resp);
    } else {
        fprintf(stdout, "Protocol: %d, url: %s, success: %d\n", resp->protocol,
                resp->url, resp->success);
    }
}

void response_listener_add_handler(EventListener *thiz, int protocol, void *data, ResponseHandler handler)
{
    EventListenerPriv *priv = NULL;
    return_if_fail(thiz != NULL && protocol >= 0 && protocol < MAX_PROTCOL);
    priv = (EventListenerPriv*)thiz->priv;
    priv->handlers[protocol].handler = handler;
    priv->handlers[protocol].data = data;
}

void response_listener_remove_handler(EventListener *thiz, int protocol)
{
    EventListenerPriv *priv = NULL;
    return_if_fail(thiz != NULL && protocol >= 0 && protocol < MAX_PROTCOL);
    priv = (EventListenerPriv*)thiz->priv;
    priv->handlers[protocol].data = NULL;
    priv->handlers[protocol].handler = NULL;
}
