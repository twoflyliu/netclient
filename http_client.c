/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 05 20:40:02
*/
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#include "util.h"
#include "http_client.h"
#include "notifier.h"
#include "socket.h"
#include "logger.h"
#include "string_buffer.h"

static void _response_set_error(HttpResponse *resp, const char *fmt, ...);
static void _response_set_success(HttpResponse *resp, StringBuffer *all_resp);

#define __HC_LOG(log, priv, fmt, ...) \
    notifier_##log(priv->notifier, PROTOCOL_HTTP, priv->url, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define HC_FATAL(priv, fmt, ...) \
    __HC_LOG(fatal, priv, fmt, ##__VA_ARGS__)

// HC_ERROR 输出日志消息会立即返回
#define HC_ERROR2(priv, fmt, ...)  {\
        *fd = -1;\
        *want_read = 0;\
        *want_write = 0;\
        __HC_LOG(error, priv, fmt, ##__VA_ARGS__); \
        _response_set_error(&priv->response, fmt, ##__VA_ARGS__); \
        return STAT_FINAL; \
    }

#define HC_ERROR(priv, fmt, ...)  {\
        __HC_LOG(error, priv, fmt, ##__VA_ARGS__); \
        _response_set_error(&priv->response, fmt, ##__VA_ARGS__); \
        return STAT_FINAL; \
    }

#define HC_WARN(priv, fmt, ...) \
    __HC_LOG(warn, priv, fmt, ##__VA_ARGS__)

#define HC_INFO(priv, fmt, ...) \
    __HC_LOG(info, priv, fmt, ##__VA_ARGS__)

#define HC_DEBUG(priv, fmt, ...) \
    __HC_LOG(debug, priv, fmt, ##__VA_ARGS__)

static int http_client_handle(ProtocolClient *thiz, int *fd, int *want_read, int *want_write);
static ClientStat http_client_on_idle(ProtocolClient *thiz, int *fd, int *want_read, int *want_write);
static int http_client_get_sockfd(ProtocolClient *thiz);
static void http_client_set_timeout(ProtocolClient *thiz, int sec);
static void http_client_set_retry_count(ProtocolClient *thiz, int retry_count);
static int http_client_get_resp_incre_len(ProtocolClient *thiz);
static void http_client_destroy(ProtocolClient *thiz);

enum {
    STAT_FINAL =0, //! 要兼容CLIENT_STAT_FINAL 
    STAT_INIT,
    STAT_CONNECTING,
    STAT_SENDING_REQUEST,
    STAT_RECEIVE_RESPONSE,
};

typedef struct _ClientPrivInfo {
    int timeout;
    int retry_count;
    int method;
    char url[PATH_MAX];

    int stat;
    time_t tprev; //上次执行时间

    Url rurl;
    Socket *socket;
    HttpHeaders *request_headers;
    Notifier *notifier;

    StringBuffer *buffer;           //!< 可以自动增加尺寸的缓冲区
    int to_send_request_index;     //!< 一个要发送的请求字符串
    int prev_resp_len;             //!< 保存前一次响应长度，用来统计下载速度

    HttpResponse response;
} ClientPrivInfo;

static void _setup_request_headers(ClientPrivInfo *priv, char **req_headers);
ProtocolClient *http_protocol_client_create(Notifier *notifier,
        void UNUSED *arg, Request *request)
{
    ProtocolClient *thiz = (ProtocolClient*)calloc(sizeof(ProtocolClient) + sizeof(ClientPrivInfo), 1);
    if (thiz != NULL) {
        HttpRequest *req = (HttpRequest*)request;
        ClientPrivInfo *priv = (ClientPrivInfo*)thiz->priv;

        thiz->handle = http_client_handle;
        thiz->get_sockfd = http_client_get_sockfd;
        thiz->set_timeout = http_client_set_timeout;
        thiz->set_retry_count = http_client_set_retry_count;
        thiz->get_resp_incre_len = http_client_get_resp_incre_len;
        thiz->on_idle = http_client_on_idle;
        thiz->destroy = http_client_destroy;

        priv->timeout = -1; // 表示不限时
        priv->retry_count = 0; // 表示不进行尝试
        priv->socket = NULL;
        priv->notifier = notifier;
        priv->stat = STAT_INIT; 
        priv->buffer = string_buffer_create(100);
        priv->prev_resp_len = 0;
        priv->to_send_request_index = -1;

        priv->method = req->method;
        _setup_request_headers(priv, req->headers); // 只会构建一次
        strcpy(priv->url, req->base.url);

        memset(&priv->response, 0, sizeof(priv->response));
        priv->response.base.protocol = PROTOCOL_HTTP;
        priv->response.base.url = priv->url;
    }
    return thiz;
}

static void http_client_destroy(ProtocolClient *thiz)
{
    ClientPrivInfo *priv = NULL;
    return_if_fail(thiz != NULL);
    priv = (ClientPrivInfo*)thiz->priv;
    if (priv->request_headers) http_headers_destroy(priv->request_headers);
    if (priv->response.response_headers != NULL) http_headers_destroy(priv->response.response_headers);
    if (priv->socket) socket_destroy(priv->socket);
    if (priv->buffer) string_buffer_destroy(priv->buffer);
    free(thiz);
}


static int http_client_get_sockfd(ProtocolClient *thiz)
{
    ClientPrivInfo *priv = NULL;
    return_value_if_fail(thiz != NULL, -1);
    priv = (ClientPrivInfo*)thiz->priv;
    return (priv->socket == NULL) ? -1 : socket_get_fd(priv->socket);
}

static void http_client_set_timeout(ProtocolClient *thiz, int sec)
{
    ClientPrivInfo *priv = NULL;
    return_if_fail(thiz != NULL);
    priv = (ClientPrivInfo*)thiz->priv;
    priv->timeout = sec;
}

static void http_client_set_retry_count(ProtocolClient *thiz, int retry_count)
{
    ClientPrivInfo *priv = NULL;
    return_if_fail(thiz != NULL);
    priv = (ClientPrivInfo*)thiz->priv;
    priv->retry_count = retry_count;
}

// 将常见的{"Host", "www.baidu.com", NULL} 构造成HttpHeaders对象
static void _setup_request_headers(ClientPrivInfo *priv, char **req_headers)
{
    HttpHeaders *headers  = NULL;
    headers = http_headers_create();
    if (req_headers != NULL) {
        while (*req_headers != NULL) {
            return_if_fail(*(req_headers + 1) != NULL); // 下一个元素就是value, 当前是key
            http_headers_add(headers, *req_headers, *(req_headers + 1));
            req_headers += 2;
        }
    }
    priv->request_headers = headers;
}

static int _http_client_connect(ClientPrivInfo *priv, int *fd, int *want_read, int *want_write);
static int _http_client_send_request(ClientPrivInfo *priv, int *fd, int *want_read, int *want_write);
static int _http_client_receive_response(ClientPrivInfo *priv, int *fd, int *want_read, int *want_write);

static int _http_client_init(ClientPrivInfo *priv);

static int _http_client_redirect(ClientPrivInfo *priv, int *fd, int *want_read, int *want_write);
static int _http_client_refresh(ClientPrivInfo *priv, int *fd, int *want_read, int *want_write);
static int _http_client_is_timeout(ClientPrivInfo *priv);
/*static int _http_timeout_reconnect(ClientPrivInfo *priv);*/

static int http_client_handle(ProtocolClient *thiz, int *fd, int *want_read, int *want_write)
{
    ClientPrivInfo *priv = NULL;
    return_value_if_fail(thiz != NULL && fd != NULL && want_read != NULL &&
            want_write != NULL, STAT_FINAL);
    priv = (ClientPrivInfo*)thiz->priv;

    if (STAT_INIT == priv->stat) { // 是个临时状态, 单独独立出这个状态，主要为了方便处理重定向等类似操作
        if (STAT_FINAL == _http_client_init(priv)) {
            return STAT_FINAL; // 表示初始化时候发生错误
        } // 内部会解析出url的各个状态，和初始化开始时间
        priv->stat = STAT_CONNECTING;
    }

    switch(priv->stat) {
        case STAT_CONNECTING:
            _http_client_connect(priv, fd, want_read, want_write);
            break;
        case STAT_SENDING_REQUEST:
            _http_client_send_request(priv, fd, want_read, want_write);
            break;
        case STAT_RECEIVE_RESPONSE:
            _http_client_receive_response(priv, fd, want_read, want_write);
        default:
            break;
    }

    // 检测失败或者重定向等情况
    if (STAT_FINAL == priv->stat) {
        if (priv->response.base.success) {
            if (200 == priv->response.code) {
                goto end_test_do;
            } else if (3 == (priv->response.code / 100)) { //状态码形如301 302...是重定向
                _http_client_redirect(priv, fd, want_read, want_write);
            }
        } else { // 因为其他问题出错(非网络问题)，无法通过尝试来解决
            goto end_test_do;
        }
    } 

    // 要么是等待接收超时，要么是服务器临时出现问题，都会进行重连
    if (STAT_FINAL == priv->stat || _http_client_is_timeout(priv)) {
        if (priv->retry_count > 0) {
            --priv->retry_count;
            _http_client_refresh(priv, fd, want_read, want_write);
        }
    }

end_test_do:
    if (STAT_FINAL == priv->stat) { // 如果终止的话，则要发送响应事件
        ResponseEvent event;
        event.base.event_type = EVENT_TYPE_RESPONSE;
        event.base.protocol = PROTOCOL_HTTP;
        event.base.url = priv->url;
        event.response = (Response*)&priv->response;
        notifier_notify(priv->notifier, (Event*)&event);
    }
    return priv->stat;
}

static int http_client_get_resp_incre_len(ProtocolClient *thiz)
{
    ClientPrivInfo *priv = NULL;
    int len = 0;
    int size;
    return_value_if_fail(thiz != NULL, 0);
    // 只有接收状态，返回的值才有效
    if (STAT_RECEIVE_RESPONSE == priv->stat) {
        priv = (ClientPrivInfo*)thiz;
        size = (int)string_buffer_size(priv->buffer);
        len = size - priv->prev_resp_len;
        priv->prev_resp_len = size;
        return len > 0 ? len : 0;
    } 
    return 0;
}

static ClientStat http_client_on_idle(ProtocolClient *thiz, int *fd, int *want_read, int *want_write)
{
    ClientPrivInfo *priv = NULL;
    return_value_if_fail(thiz != NULL, (ClientStat)STAT_FINAL);
    priv = (ClientPrivInfo*)thiz->priv;
    HC_DEBUG(priv, "on_idle: url [%s]", priv->url);

    if (_http_client_is_timeout(priv)) {
        if (priv->retry_count > 0) {
            --priv->retry_count;
            _http_client_refresh(priv, fd, want_read, want_write);
        } else {
            _response_set_error(&priv->response, "Timeout");
            priv->stat = STAT_FINAL;
        }
    }

    if (STAT_FINAL == priv->stat) { // 如果终止的话，则要发送响应事件
        ResponseEvent event;
        event.base.event_type = EVENT_TYPE_RESPONSE;
        event.base.protocol = PROTOCOL_HTTP;
        event.base.url = priv->url;
        event.response = (Response*)&priv->response;
        notifier_notify(priv->notifier, (Event*)&event);
    }
    return (ClientStat)priv->stat;
}

static void _http_client_request_string_init(ClientPrivInfo *thiz); // 初始化要发送的请求

static int _http_client_init(ClientPrivInfo *priv)
{
    Url *url = &priv->rurl;

    // 解析url
    url_parse(priv->url, url);
    if (!priv->rurl.ok) {
        HC_ERROR(priv, "Parse url [%s] failed!: %s\n", priv->url, priv->rurl.errmsg); // 会退出整个handle
    }

    // 更新请求头的Host属性
    http_headers_remove(priv->request_headers, "Host");
    http_headers_add(priv->request_headers, "Host", url->host); //重定向后可能主机名也会变调，所以这儿强制更新

    // 创建socket
    if (priv->socket != NULL) {
        socket_destroy(priv->socket); // 有可能重定向做这个逻辑(所以先关掉原来的然后创建新的)
    }

    priv->socket = socket_create(0 == strcmp("https", url->scheme));
    if (url->port == -1) {
        url->port = (0 == strcmp("http", url->scheme)) ? 80 : 443;
    }
    
    // 重置时间
    time(&priv->tprev); // 初始化时间
    _http_client_request_string_init(priv); // 初始化请求字符串
    priv->prev_resp_len = 0;

    return STAT_INIT;
}

static const char* _method_to_string(int method)
{
    switch(method) {
        case HTTP_REQUEST_METHOD_GET:
            return "GET";
        case HTTP_REQUEST_METHOD_POST:
            return "POST";
        case HTTP_REQUEST_METHOD_HEAD:
            return "HEAD";
        default:
            return "UNKNOWN";
    }
}

static void _http_client_request_string_init(ClientPrivInfo *priv)
{
    char buf[BUFSIZ];
    int n;
    const char **keys;
    const char *value;

    string_buffer_clear(priv->buffer);
    n = snprintf(buf, sizeof(buf) - 1, "%s %s %s\r\n", 
            _method_to_string(priv->method), priv->rurl.path, "HTTP/1.0");
    string_buffer_append(priv->buffer, buf);

    // 构建请求头
    if (priv->request_headers != NULL && (keys = http_headers_keys(priv->request_headers)) != NULL) {
        while (*keys != NULL) {
            value = http_headers_get(priv->request_headers, *keys);
            if (value != NULL) {
                n = snprintf(buf, BUFSIZ - 1, "%s: %s\r\n", *keys, value);
                buf[n] = '\0';
                string_buffer_append(priv->buffer, buf);
            } else {
                HC_WARN(priv, "Invalid request key %s, which not contains value", *keys);
            }
            ++keys;
        }
    }
    string_buffer_append(priv->buffer, "\r\n"); //连续两个\r\n\r\n表示终止字符
    priv->to_send_request_index = 0;
}

static int _http_client_connect(ClientPrivInfo *priv, int *fd, int *want_read, int *want_write)
{
    *fd = -1;
    *want_read = *want_write = 0;
    HC_DEBUG(priv, "Try to connect server [%s]!", priv->url);
    if (socket_connect(priv->socket, priv->rurl.host, priv->rurl.port) <= 0) {
        if (!socket_should_retry(priv->socket)) {
            HC_ERROR2(priv, "Connect to server [%s] failed!: %s", priv->url, socket_error(priv->socket));
        } else {
            priv->stat = STAT_CONNECTING;
        }
    } else {
        priv->stat = STAT_SENDING_REQUEST;
        HC_INFO(priv, "Connect to server [%s] sucessfully!", priv->url);
    }

    *fd = socket_get_fd(priv->socket); // 只有建立连接后socket才有效
    *want_read = 0;
    *want_write = 1;
    return priv->stat;
}

static int _http_client_send_request(ClientPrivInfo *priv, int *fd, int *want_read, int *want_write)
{
    int n;
    size_t buffer_size;

    HC_DEBUG(priv, "Try to send request to server [%s]", priv->url);

    buffer_size = string_buffer_size(priv->buffer);
    if (0 == buffer_size || priv->to_send_request_index < 0 || 
            priv->to_send_request_index >= (int)buffer_size) {
        HC_ERROR2(priv, "Url: [%s], Invalid send request string", priv->url);
    }

    *fd = -1;
    *want_read = *want_write = 0;
    // 下面发送请求，也处理请求头太长情况
    n = socket_write(priv->socket, string_buffer_c_str(priv->buffer) + priv->to_send_request_index,
            buffer_size - priv->to_send_request_index);

    if (n > 0) {
        if (n == (int)buffer_size - priv->to_send_request_index) { //成功情况
            HC_INFO(priv, "Send request to [%s] successfully!", priv->url);
            *fd = socket_get_fd(priv->socket);
            *want_read = 1;
            *want_write = 0;
            priv->stat = STAT_RECEIVE_RESPONSE;
            string_buffer_clear(priv->buffer); // 这个时候buffer作为接收的缓冲区
            priv->to_send_request_index = -1;
        } 
    } else if (0 == n) {
        HC_ERROR2(priv, "Server [%s] was closed prematurely", priv->url);
    } else {
        if (!socket_should_retry(priv->socket)) {
            HC_ERROR2(priv, "Send request to [%s] failed: %s", priv->url, socket_error(priv->socket));
        }
    }

    if (STAT_SENDING_REQUEST == priv->stat) { // 仍然是发送状态
        *fd = socket_get_fd(priv->socket);
        *want_read = 0;
        *want_write = 1;
    }

    return priv->stat;
}

// 接收首相仍然使用priv->buffer来作为缓冲区
static int _http_client_receive_response(ClientPrivInfo *priv, int *fd, int *want_read, int *want_write)
{
    char buf[BUFSIZ+1];
    int n;

    *fd = -1;
    *want_read = *want_write = 0;

    HC_DEBUG(priv, "try to receive response from server [%s]", priv->url);
    n = socket_read(priv->socket, buf, BUFSIZ);
    if (n == 0) {
        // 表明服务器端关闭了，因为使用HTTP/1.0 或者使用HTTP/2.0，但是将Keep-Alive设置为close, 所以关闭就表示传输结束
        HC_INFO(priv, "received all response from server [%s]", priv->url);
        priv->stat = STAT_FINAL;
        _response_set_success(&priv->response, priv->buffer);
    } else if (n > 0) {
        buf[n] = '\0';
        string_buffer_append(priv->buffer, buf);
    } else {
        if (!socket_should_retry(priv->socket)) {
            HC_ERROR2(priv, "Receive from server [%s] failed: %s", priv->url, socket_error(priv->socket));
        }
    }

    if (STAT_RECEIVE_RESPONSE == priv->stat) { 
        *fd = socket_get_fd(priv->socket);
        *want_read = 1;
        *want_write = 0;
    }
    return priv->stat;
}

static void _response_set_error(HttpResponse *resp, const char *fmt, ...)
{
    char buf[BUFSIZ];
    int n;
    va_list va;

    va_start(va, fmt);
    n = vsnprintf(buf, BUFSIZ - 1, fmt, va);
    buf[n] = '\0';
    va_end(va);

    resp->base.success = 0;
    strcpy(resp->error_msg, buf);

    resp->response_content = resp->status = resp->http_version = NULL;
    if (resp->response_headers != NULL) {
        http_headers_destroy(resp->response_headers);
        resp->response_headers = NULL;
    }
}

static const char * _response_parse_status_line(HttpResponse *resp,
        const char *cursor);
static const char * _response_parse_response_headers(HttpResponse *resp,
        const char *cursor);
static void _response_parse_response_data(HttpResponse *resp,
        const char *cursor);

static void _response_set_success(HttpResponse *resp, StringBuffer *all_resp)
{
    const char *pstr;
    resp->base.success = 1;

    pstr = string_buffer_c_str(all_resp);
    assert(pstr != NULL);
    pstr = _response_parse_status_line(resp, pstr);
    assert(pstr != NULL);
    pstr = _response_parse_response_headers(resp, pstr);
    assert(pstr != NULL);
    _response_parse_response_data(resp, pstr);
}

static inline const char * _skip_white_space(const char *cursor);

// 格式是形如：HTTP/1.1 200 OK\r\n。。。格式
static const char * _response_parse_status_line(HttpResponse *resp,
        const char *cursor)
{
    cursor= _skip_white_space(cursor);
    resp->http_version =cursor;
    cursor= strchr(cursor, ' ');
    *(char*)cursor= '\0'; // 这儿主要是为了解决内存，所有的结构都保存在priv->buffer中

    ++cursor;
    cursor= _skip_white_space(cursor);
    resp->code = atoi(cursor);

    cursor= strchr(cursor, ' ');
    cursor= _skip_white_space(cursor);
    resp->status = cursor;

    cursor= strchr(cursor, '\r');
    *(char*)cursor= '\0';

    ++cursor;
    cursor= _skip_white_space(cursor);
    return cursor;
}

//解析形如:Host: www.baidu.com\r\nCookie: user=ffx\r\n\r\n
static const char * _response_parse_response_headers(HttpResponse *resp,
        const char *cursor)
{
    const char *key, *value;
    if (resp->response_headers != NULL) {
        http_headers_destroy(resp->response_headers);
    }
    resp->response_headers = http_headers_create();

    while (*cursor != '\r') {
        cursor = _skip_white_space(cursor);
        key = cursor; //获取key
        cursor = strchr(cursor, ':');
        *(char*)cursor = '\0';

        ++cursor;
        cursor = _skip_white_space(cursor);
        value = cursor; //获取value
        cursor = strchr(cursor, '\r');
        *(char*)cursor = '\0';

        http_headers_add(resp->response_headers, key, value);
        cursor += 2; // 应该定位到下一行开头
    }

    //理论上cursor后面两个字符应该是\r\n
    assert(0 == strncmp("\r\n", cursor, 2));
    cursor += 2;
    return cursor;
}

static void _response_parse_response_data(HttpResponse *resp,
        const char *cursor)
{
    cursor = _skip_white_space(cursor);
    resp->response_content = cursor;
}

static inline const char * _skip_white_space(const char *cursor)
{
    while (*cursor != '\0' && isspace(*cursor)) ++cursor;
    return cursor;
}

static int _http_client_redirect(ClientPrivInfo *priv, int *fd, int *want_read, int *want_write)
{
    const char *new_location = NULL;
    return_value_if_fail(priv->response.base.success && priv->response.response_headers != NULL, STAT_FINAL);
    new_location = http_headers_get(priv->response.response_headers, "Location");
    return_value_if_fail(new_location != NULL, STAT_FINAL);
    HC_INFO(priv, "Server from [%s] redirect to [%s]\n", priv->url, new_location);
    strcpy(priv->url, new_location);

    if (STAT_FINAL == _http_client_init(priv)) return STAT_FINAL;
    return _http_client_connect(priv, fd, want_read, want_write);
}

static int _http_client_refresh(ClientPrivInfo *priv, int *fd, int *want_read, int *want_write)
{
    _http_client_init(priv); // 因为已经走过一次init流程，所以他肯定不会失败
    return _http_client_connect(priv, fd, want_read, want_write);
}

// 如果priv->timeout <= 0都不会进行超时重连
static int _http_client_is_timeout(ClientPrivInfo *priv)
{
    time_t now;
    time(&now);
    return (priv->timeout) > 0 ? ((now - priv->tprev) > priv->timeout) : 0;
}
