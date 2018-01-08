/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 08 16:50:17
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "http_response_listener.h"
#include "response.h"
#include "response_listener.h"
#include "http_headers.h"

static void http_response_listener_handle(struct _EventListener *thiz, Event *event);
static void http_response_listener_destroy(struct _EventListener *thiz);

typedef struct _EventListenerPriv {

} EventListenerPriv;

// 只是将响应的基本信息给打印出来，对于具体的
// 响应比如http响应，需要根据自己需求来进行操作
EventListener* http_response_listener_create()
{
    EventListener* thiz = (EventListener*)malloc(sizeof(EventListener) + sizeof(EventListenerPriv));
    if (thiz != NULL) {
        thiz->handle = http_response_listener_handle;
        thiz->destroy = http_response_listener_destroy;
    }
    return thiz;
}

static void http_response_listener_destroy(struct _EventListener *thiz)
{
    free(thiz);
}

static inline void _show_status_line(HttpResponse *resp);
static inline void _show_headers(HttpResponse *resp);
static inline void _show_content(HttpResponse *resp);

// 只是输出response的基本信息，对于确定的协议，要根据你的实际需求来操作
static void http_response_listener_handle(struct _EventListener UNUSED *thiz, Event *event)
{
    ResponseEvent *resp_event = NULL;
    HttpResponse *resp;

    if (event->event_type != EVENT_TYPE_RESPONSE) return;
    resp_event = (ResponseEvent*)event;
    if (resp_event->response->protocol != PROTOCOL_HTTP) return; //只处理http协议的响应，不处理其他协议响应
    resp = (HttpResponse*)resp_event->response;

    printf("\nHTTP PROTCOL: url: %s\n\n", resp->base.url);
    printf("success: %d\n", resp->base.success);

    if (resp->base.success) {
        _show_status_line(resp);
        _show_headers(resp);
        _show_content(resp);
    } else {
        printf("fail message: %s\n", resp->error_msg);
    }
}

static inline void _show_status_line(HttpResponse *resp)
{

    printf("http version: %s\n", resp->http_version);
    printf("code: %d\n", resp->code); 
    printf("status: %s\n\n", resp->status);
}

static inline void _show_headers(HttpResponse *resp)
{
    const char **keys = http_headers_keys(resp->response_headers);
    const char *value = NULL;
    if (NULL == keys) return;
    while (*keys != NULL) {
        value = http_headers_get(resp->response_headers, *keys);
        printf("%s: %s\n", *keys, value);
        ++keys;
    }
    printf("\n");
}

static inline void _show_content(HttpResponse *resp)
{
    printf("content length: %d\n", (int)strlen(resp->response_content));
}
