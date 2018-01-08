/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 08 16:12:30
*/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "util.h"
#include "logger.h"
#include "logger_listener.h"

static void logger_event_listener_handle(struct _EventListener *thiz, Event *event);
static void logger_event_listener_destroy(struct _EventListener *thiz);

typedef struct _EventListenerPriv {
    int logger_level;
    FILE *fp;
} EventListenerPriv;

EventListener* logger_listener_create(int logger_level, FILE *fp)
{
    EventListener *thiz = (EventListener*)malloc(sizeof(EventListener) + sizeof(EventListenerPriv));
    EventListenerPriv *priv = NULL;
    if (thiz != NULL) {
        thiz->handle = logger_event_listener_handle;
        thiz->destroy = logger_event_listener_destroy;

        priv = (EventListenerPriv*)thiz->priv;
        priv->logger_level = logger_level;
        priv->fp = (fp == NULL) ? stdout : fp;
    }
    return thiz;
}

static void logger_event_listener_destroy(struct _EventListener *thiz)
{
    EventListenerPriv *priv = (EventListenerPriv*)thiz;
    if (thiz != NULL) {
        if (priv->fp != stdout || priv->fp != stderr) fclose(priv->fp); 
        free(thiz);
    }
}

static inline void _log(EventListenerPriv *priv, LoggerEvent *event);

static void logger_event_listener_handle(struct _EventListener *thiz, Event *event)
{
    EventListenerPriv *priv = NULL;
    return_if_fail(thiz != NULL && event != NULL);
    // 值处理LoggerEvent
    if (event->event_type != EVENT_TYPE_LOGGER) return;
    priv = (EventListenerPriv*)thiz->priv;
    _log(priv, (LoggerEvent*)event);
}

// 这个日志监听器，如果想写复杂，可以非常复杂，但是也可以使用第三方日志库来实现
// 功能强大的日志操作，这儿展示简单输出
static inline void _log(EventListenerPriv *priv, LoggerEvent *event)
{
    time_t now;
    char buf[BUFSIZ];
    if (priv->logger_level <= event->level) {
        time(&now);
        ctime_r(&now, buf);
        fprintf(priv->fp, "%s [%s]: %s", buf, 
                logger_level_string((LoggerLevel)event->level), event->message);
        fflush(priv->fp); // 只是简单输出
    }
}
