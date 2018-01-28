#ifndef LOGGER_H_H
#define LOGGER_H_H

#include <stddef.h>

#include "event.h"
#include "notifier.h"
#include "util.h"

typedef enum _LoggerLevel {
    LOGGER_FATAL=0, //!< 致命错误要退出程序
    LOGGER_ERROR, //!< 普通错误
    LOGGER_WARN,  //!< 警告
    LOGGER_INFO,  //!< 普通信息
    LOGGER_DEBUG, //!< 调试信息
} LoggerLevel;

/*!
 *  \brief LoggerEvent 一个日志事件
 */
typedef struct _LoggerEvent {
    Event base;
    int level;
    const char *file;
    int line;
    const char *message;
} LoggerEvent;

const char * logger_level_string(LoggerLevel level);

/*!
 *  \brief logger_level_from_string 将字符串转换为LoggerLevel类型
 *  \retval 如果返回值>=0，表示成功，否则表示未知级别字符串  
 */
int logger_level_from_string(const char *level_string); 

void notifier_fatal(Notifier *notifier, Protocol protocol, const char *url,
        const char *file, int line, const char *fmt, ...);

void notifier_error(Notifier *notifier, Protocol protocol, const char *url,
        const char *file, int line, const char *fmt, ...);

void notifier_warn(Notifier *notifier, Protocol protocol, const char *url,
        const char *file, int line, const char *fmt, ...);

void notifier_info(Notifier *notifier, Protocol protocol, const char *url,
        const char *file, int line, const char *fmt, ...);

void notifier_debug(Notifier *notifier, Protocol protocol, const char *url,
        const char *file, int line, const char *fmt, ...);

#endif //LOGGER_H_H
