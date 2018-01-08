#ifndef NOTIFIER_H_H
#define NOTIFIER_H_H

#ifndef EVENT_H_H
#include "event.h"
#endif

struct _EventListener;

struct _Notifier;
typedef struct _Notifier Notifier;


Notifier *notifier_create();
void notifier_destroy(Notifier *thiz);

void notifier_add_event_listener(Notifier *thiz, int protocol, 
        struct _EventListener *event_listener); //!< 增加事件监听器

void notifier_remove_event_listener(Notifier *thiz, int protocol, 
        struct _EventListener *event_listener); //!< 移除事件监听器

void notifier_notify(Notifier *thiz, Event *event); //!< 通知知道有真正的响应

Notifier *notifier_create(void); //!< 工厂函数
void notifier_destroy(Notifier *thiz); //!< 销毁函数

#endif //NOTIFIER_H_H
