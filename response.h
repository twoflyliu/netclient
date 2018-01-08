#ifndef RESPONSE_H_H
#define RESPONSE_H_H

#include "protocol.h"
#include "event.h"

/*!
 * \brief Response 响应对象
 */
typedef struct _Response {
    Protocol protocol;          //!< 协议
    const char *url;            //!< 请求的url
    int success;                 //!< 状态码，比如成功，失败
} Response;

typedef struct _ResponseEvent {
    Event base;
    Response *response;
} ResponseEvent;

#endif //RESPONSE_H_H
