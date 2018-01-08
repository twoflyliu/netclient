#ifndef REQUEST_H_H
#define REQUEST_H_H

#include "protocol.h"

/*!
 * \brief Request 一个请求的抽象
 */
typedef struct _Request {
    Protocol protocol;
    const char *url;
} Request;

#endif //REQUEST_H_H
