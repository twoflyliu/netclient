/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 07 10:00:31
*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "http_headers.h"
#include "list.h"

char *strdup(const char *s);

typedef struct _HttpHeader {
    char *key;
    char *value;
    size_t value_capacity;
} HttpHeader;

static void http_header_destroy(HttpHeader *header)
{
    if (header != NULL) {
        if (header->key != NULL) free(header->key);
        if (header->value != NULL) free(header->value);
        free(header);
    }
}

static HttpHeader *http_header_create(const char *key, const char *value)
{
    HttpHeader *thiz = NULL;
    return_value_if_fail(key != NULL && value != NULL, NULL);
    thiz = (HttpHeader*)malloc(sizeof(HttpHeader));
    if (thiz != NULL) {
        thiz->key = strdup(key);
        thiz->value = strdup(value);
        thiz->value_capacity = strlen(value) + 1; // 主要是方便修改时候不在重新分配内存
    }
    return thiz;
}

static void http_header_append_value(HttpHeader *thiz, const char *value)
{
    size_t need_space = strlen(thiz->value) + strlen(value) + 1;
    if (need_space > thiz->value_capacity) {
        need_space = need_space + (need_space >> 1); // 如果添加一次的，预计还会增加多次，所以多分配一点内存
        char *new_value = (char*)realloc(thiz->value, need_space);
        if (new_value != NULL) {
            thiz->value = new_value;
            thiz->value_capacity = need_space;
        } else {
            assert(0 && "realloc failed!");
        }
    }
    strcat(thiz->value, value);
}

struct _HttpHeaders {
    List *headers;
    char keys[1024];
};

HttpHeaders *http_headers_create()
{
    HttpHeaders *thiz = (HttpHeaders*)malloc(sizeof(HttpHeaders));
    if (thiz != NULL) {
        thiz->headers = list_create((DataDestroyFunc)http_header_destroy);
    }
    return thiz;
}

void http_headers_destroy(HttpHeaders *thiz)
{
    if (thiz->headers != NULL) list_destroy(thiz->headers);
    thiz->headers = NULL;
    free(thiz);
}

static int http_header_compare(const char *key, HttpHeader *header)
{
    return strcmp(key, header->key);
}

void http_headers_add(HttpHeaders *thiz, const char *key, const char *value)
{
    HttpHeader *header = NULL;
    return_if_fail(thiz != NULL && key != NULL && value != NULL && thiz->headers != NULL);
    // 线查找是否存在，如果存在，则将内容追加到尾部否则才会创建新的
    header = (HttpHeader*)list_find(thiz->headers, (DataCompareFunc)http_header_compare, (void*)key);
    if (header == NULL) {
        header = http_header_create(key, value);
        return_if_fail(header != NULL);
        list_push_back(thiz->headers, header);
    } else {
        http_header_append_value(header, value);
    }
}

void http_headers_remove(HttpHeaders *thiz, const char *key)
{
    Iterator *iter = NULL;
    return_if_fail(thiz != NULL && key != NULL);
    // 使用迭代器既不会有太大损耗又不会暴露数据
    for (iter = list_begin(thiz->headers); !iterator_is_done(iter); iterator_next(iter)) {
        HttpHeader *header = (HttpHeader*)iterator_data(iter);
        if (0 == strcmp(key, header->key)) {
            iterator_erase(iter);
            break;
        }
    }
    if (iter) iterator_destroy(iter);
}

const char** http_headers_keys(HttpHeaders *thiz)
{
    Iterator *iter = NULL;
    HttpHeader *header = NULL;
    int offset = 0;
    return_value_if_fail(thiz != NULL, NULL);
    thiz->keys[0] = '\0';
    for (iter = list_begin(thiz->headers); !iterator_is_done(iter); iterator_next(iter)) {
        header = (HttpHeader*)iterator_data(iter);
        strcpy(thiz->keys + offset, header->key);
        offset += strlen(header->key) + 1; // 跳过'\0'
    }
    if (iter) iterator_destroy(iter);
    return (const char**)thiz->keys;
}

const char* http_headers_get(HttpHeaders *thiz, const char *key)
{
    HttpHeader *header = NULL;
    return_value_if_fail(thiz != NULL && key != NULL&& thiz->headers != NULL, NULL);
    // 线查找是否存在，如果存在，则将内容追加到尾部否则才会创建新的
    header = (HttpHeader*)list_find(thiz->headers, (DataCompareFunc)http_header_compare, (void*)key);
    return (header != NULL) ? header->value : NULL;
}

#ifdef TEST_HTTP_HEADERS

#ifdef NDEBUG
#define NDEBUG
#endif

int main(void)
{
    HttpHeaders *headers = (HttpHeaders*)http_headers_create();
    http_headers_add(headers, "Host", "www.baidu.com");
    assert(0 == strcmp("www.baidu.com", http_headers_get(headers, "Host")));
    assert(NULL == http_headers_get(headers, "Google"));

    http_headers_remove(headers, "Host");
    assert(NULL == http_headers_get(headers, "Host"));

    http_headers_add(headers, "Cookie", "username=ffx;");
    http_headers_add(headers, "Cookie", "password=ffx;");
    assert(0 == strcmp("username=ffx;password=ffx;", http_headers_get(headers, "Cookie")));

    http_headers_destroy(headers);
    return 0;
}

#endif
