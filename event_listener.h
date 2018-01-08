#ifndef EVENT_LISTENER_H_H
#define EVENT_LISTENER_H_H

#ifndef EVENT_H_H
#include "event.h"
#endif

#include <stddef.h>

#ifndef UTIL_H_H
#include "util.h"
#endif


typedef struct _EventListener {
    void (*handle)(struct _EventListener *thiz, Event *event);
    void (*destroy)(struct _EventListener *thiz);
    char priv[0];
} EventListener;


#endif //EVENT_LISTENER_H_H
