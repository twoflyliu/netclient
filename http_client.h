#ifndef HTTP_CLIENT_H_H
#define HTTP_CLIENT_H_H

#include "protocol_client.h"
#include "request.h"
#include "http_headers.h"

enum {
    HTTP_REQUEST_METHOD_GET,
    HTTP_REQUEST_METHOD_POST,
    HTTP_REQUEST_METHOD_HEAD,
};

/*!
 * \brief HttpRequest 表示一个http请求
 */
typedef struct _HttpRequest {
    Request base;           //!< 相当于继承Request
    int method;
    char** headers;         //!< 请求头
    void *data;             //!< 请求数据，只有当是POST请求时候才有效
    int datalen;            //!< data的长度
} HttpRequest;

#include "response.h"
typedef struct _HttpResponse {
    Response base;
    char error_msg[100];
    int code; //!< 状态码
    const char  *status;           //!< 比如Ok..., 下面四个参数前提是base.success = 1
    const char  *http_version;     //!< http版本
    HttpHeaders *response_headers; //!< 响应头
    const char  *response_content; //!< 响应数据
} HttpResponse;


ProtocolClient *http_protocol_client_create(Notifier *,
        void *arg, Request *request);

#endif //HTTP_CLIENT_H_H
