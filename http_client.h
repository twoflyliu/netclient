#ifndef HTTP_CLIENT_H_H
#define HTTP_CLIENT_H_H

#include "protocol_client.h"
#include "request.h"
#include "http_headers.h"

enum {
    HTTP_REQUEST_METHOD_GET = 0,
    HTTP_REQUEST_METHOD_POST,
    HTTP_REQUEST_METHOD_HEAD,
};

enum {
    HTTP_HEADERS_TYPE_NONE,
    HTTP_HEADERS_TYPE_MAP,
    HTTP_HEADERS_TYPE_ARR,
};

/*!
 * \brief HttpRequest 表示一个http请求
 */
typedef struct _HttpRequest {
    Request base;           //!< 相当于继承Request
    int method;
    struct {
        int headers_type;
        union {
            HttpHeaders *map_headers;
            char** array_headers;         //!< 请求头
        };
    };
    const void *data;             //!< 请求数据，只有当是POST请求时候才有效
    size_t datalen;            //!< data的长度
} HttpRequest;

//void http_request_setup_from_array_headers(HttpRequest *thiz, const char *url, const void *data,
        //size_t datalen, char **array_headers);
//void http_request_setup_from_map_headers(HttpRequest *thiz, const char *url, const void *data,
        //size_t datalen, HttpHeaders *headers);

// 只会清理内部资源，不会free(thiz)
void http_request_teardown(HttpRequest *thiz);

// 调用http_request_teardown, 返回free(thiz)
void http_request_destroy(HttpRequest *thiz);


#include "response.h"
typedef struct _HttpResponse {
    Response base;
    char error_msg[100];
    int code; //!< 状态码
    const char  *status;           //!< 比如Ok..., 下面四个参数前提是base.success = 1
    const char  *http_version;     //!< http版本
    HttpHeaders *response_headers; //!< 响应头
    const char  *response_content; //!< 响应数据
    int request_method;            //!< 之前的请求方法, 用来区分同个url，但是请求方法不同的响应
} HttpResponse;


ProtocolClient *http_protocol_client_create(Notifier *,
        void *arg, Request *request);

/*!
 * \brief http_method_from_string 将请求方法字符串转换为int类型
 * \param [in] method 请求方法，可以是GET,POST,HEAD
 * \retval 会根据对应的method返回对应的HTTP_REQUEST_METHOD_[具体方法], 如果method不在指定方法列表中，则返回-1
 */
int http_request_method_from_string(const char *method);

#define DEFAULT_USER_AGENT "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/62.0.3202.62 Safari/537.36"
#define DEFAULT_POST_CONTENT_TYPE "application/x-www-form-urlencoded"

#endif //HTTP_CLIENT_H_H
