/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 08 15:59:09
*/
#include <string.h>
#include "net_client.h"
#include "http_client.h"

#include "logger_listener.h"
#include "http_response_listener.h"

void download(NetClient *client, const char *url)
{
    HttpRequest request;
    const char *headers[] = {"User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/55.0.2883.87 Safari/537.36", NULL};
    memset(&request, 0, sizeof(request));
    request.base.protocol = PROTOCOL_HTTP;
    request.base.url = url;
    request.headers_type = HTTP_HEADERS_TYPE_ARR;
    request.array_headers = (char**)headers;
    request.method = HTTP_REQUEST_METHOD_GET;
    net_client_push_request(client, (Request*)&request);
    http_request_teardown(&request);
}

int main(void)
{
    NetClient *client = net_client_create();

    // 注册http协议
    net_client_register_protocol(client, PROTOCOL_HTTP,
            http_protocol_client_create, NULL);

    // 注册两个监听器，一个是日志监听器，一个是响应监听器
    // 对于日志，不关心具体协议类型，他监听所有的协议类型
    net_client_add_event_listener(client, PROTOCOL_ALL, 
            logger_listener_create(LOGGER_INFO, NULL));

    net_client_add_event_listener(client, PROTOCOL_HTTP,
            http_response_listener_create());

    // 进行下载
    download(client, "https://www.so.com/");
    download(client, "http://www.37zw.net/");
    download(client, "http://www.37zw.com/");
    
    net_client_do_all(client); // 下载所有内容
    net_client_destroy(client);
    return 0;
}
