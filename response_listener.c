/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 08 16:34:09
*/
#include <stdlib.h>
#include <stdio.h>

#include "response.h"
#include "response_listener.h"

static void response_listener_handle(struct _EventListener *thiz, Event *event);
static void response_listener_destroy(struct _EventListener *thiz);

typedef struct _EventListenerPriv {

} EventListenerPriv;

// 只是将响应的基本信息给打印出来，对于具体的
// 响应比如http响应，需要根据自己需求来进行操作
EventListener* response_listener_create()
{
    EventListener* thiz = (EventListener*)malloc(sizeof(EventListener) + sizeof(EventListenerPriv));
    if (thiz != NULL) {
        thiz->handle = response_listener_handle;
        thiz->destroy = response_listener_destroy;
    }
    return thiz;
}

static void response_listener_destroy(struct _EventListener *thiz)
{
    free(thiz);
}

// 只是输出response的基本信息，对于确定的协议，要根据你的实际需求来操作
static void response_listener_handle(struct _EventListener UNUSED *thiz, Event *event)
{
    ResponseEvent *resp_event = NULL;
    Response *resp;

    if (event->event_type != EVENT_TYPE_RESPONSE) return;
    resp_event = (ResponseEvent*)event;
    resp = resp_event->response;

    fprintf(stdout, "Protocol: %d, url: %s, success: %d\n", resp->protocol,
            resp->url, resp->success);
}
