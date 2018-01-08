#ifndef EVENT_H_H
#define EVENT_H_H

#include "protocol.h"

enum {
    EVENT_TYPE_LOGGER = 0,
    EVENT_TYPE_RESPONSE,
};

/*!
 *  \brief Event 表示事件，相当于所有事件的基类，只是一个占位符号
 */
typedef struct _Event {
    Protocol protocol;
    int event_type;  // 标识事件类型，才好分类
    const char *url;
} Event;


#endif //EVENT_H_H
