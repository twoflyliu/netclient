/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 08 10:22:26
*/
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "logger.h"

static inline void notifier_log(int logger_level, Notifier *notifier, Protocol protocol, const char *url,
        const char *file, int line, const char *format, va_list va)
{
    LoggerEvent event;
    char message[BUFSIZ];
    int n;

    event.base.event_type = EVENT_TYPE_LOGGER;
    event.base.protocol = protocol;
    event.base.url = url;

    event.file = file;
    event.line = line;
    event.level = logger_level;

    n = vsnprintf(message, sizeof(message) - 1, format, va);
    if ('\n' != message[n]) message[n++] = '\n'; // 保证每行日志占用一行
    message[n] = '\0';
    event.message = message;
    notifier_notify(notifier, (Event*)&event);
}


void notifier_fatal(Notifier *notifier, Protocol protocol, const char *url,
        const char *file, int line, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    notifier_log(LOGGER_FATAL, notifier, protocol, url, file, line, fmt, va);
    va_end(va);
    exit(1);
}

void notifier_error(Notifier *notifier, Protocol protocol, const char *url,
        const char *file, int line, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    notifier_log(LOGGER_ERROR, notifier, protocol, url, file, line, fmt, va);
    va_end(va);
}

void notifier_warn(Notifier *notifier, Protocol protocol, const char *url,
        const char *file, int line, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    notifier_log(LOGGER_WARN, notifier, protocol, url, file, line, fmt, va);
    va_end(va);
}

void notifier_info(Notifier *notifier, Protocol protocol, const char *url,
        const char *file, int line, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    notifier_log(LOGGER_INFO, notifier, protocol, url, file, line, fmt, va);
    va_end(va);
}

void notifier_debug(Notifier *notifier, Protocol protocol, const char *url,
        const char *file, int line, const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    notifier_log(LOGGER_DEBUG, notifier, protocol, url, file, line, fmt, va);
    va_end(va);
}

const char * logger_level_string(LoggerLevel level)
{
    switch(level) {
        case LOGGER_FATAL:
            return "FATAL";
        case LOGGER_ERROR:
            return "ERROR";
        case LOGGER_WARN:
            return "WARN";
        case LOGGER_INFO:
            return "INFO";
        case LOGGER_DEBUG:
            return "DEBUG";
        default:
            return "UNKOWN";
    }
}
