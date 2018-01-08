/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 06 10:24:49
*/
#include <stdlib.h>

#include "util.h"
#include "notifier.h"
#include "list.h"
#include "event_listener.h"

#include "protocol.h"
#include "notifier.h"

struct _Notifier
{
    List* listeners[MAX_PROTCOL];
};

Notifier *notifier_create()
{
    return (Notifier*)calloc(sizeof(Notifier), 1);
}

void notifier_destroy(Notifier *thiz)
{
    return_if_fail(thiz != NULL);
    for (int i = 0; i < MAX_PROTCOL; i++) {
        if (thiz->listeners[i] != NULL) {
            list_destroy(thiz->listeners[i]);
        }
    }
    free(thiz);
}

static inline List* notifier_get_list(Notifier *thiz, int protocol)
{
    List *list = thiz->listeners[protocol];
    if (NULL == list) {
        list = thiz->listeners[protocol] = list_create((DataDestroyFunc)event_listener_destroy);
    }
    return list;
}

void notifier_add_event_listener(Notifier *thiz, int protocol, 
        struct _EventListener *event_listener)
{
    List *list;
    return_if_fail(thiz != NULL && protocol < MAX_PROTCOL && event_listener != NULL);
    list = notifier_get_list(thiz, protocol);
    return_if_fail(list != NULL);
    list_push_back(list, event_listener);
}

void notifier_remove_event_listener(Notifier *thiz, int protocol, 
        struct _EventListener *event_listener)
{
    List *list;
    return_if_fail(thiz != NULL && protocol < MAX_PROTCOL && event_listener != NULL);
    list = notifier_get_list(thiz, protocol);
    return_if_fail(list != NULL && list_size(list) > 0);
    list_erase(list, event_listener);
}

// 这儿
static void _notifier_notify(Notifier *thiz, Protocol protocol, Event *event)
{
    List *list = thiz->listeners[protocol];
    if (NULL == list) return;
    for (Iterator *iter = list_begin(list); !iterator_is_done(iter); iterator_next(iter)) {
        EventListener *listener = (EventListener*)iterator_data(iter);
        listener->handle(listener, event);
    }
}


void notifier_notify(Notifier *thiz, Event *event)
{
    return_if_fail(thiz != NULL && event != NULL);
    _notifier_notify(thiz, PROTOCOL_ALL, event); //PROTOCOL_ALL监听器接收所有类型的事件
    if (event->protocol != PROTOCOL_ALL) {
        _notifier_notify(thiz, event->protocol, event); //分发给所属事件类型监听器
    }
}
