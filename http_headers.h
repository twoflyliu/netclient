#ifndef HTTP_HEADERS_H_H
#define HTTP_HEADERS_H_H

struct _HttpHeaders;
typedef struct _HttpHeaders HttpHeaders;

HttpHeaders *http_headers_create();
void http_headers_destroy(HttpHeaders *headers);
void http_headers_add(HttpHeaders *thiz, const char *key, const char *value);
void http_headers_remove(HttpHeaders *thiz, const char *key);
const char** http_headers_keys(HttpHeaders *thiz);
const char* http_headers_get(HttpHeaders *thiz, const char *key);
int http_headers_has(HttpHeaders *thiz, const char *key);

#endif //HTTP_HEADERS_H_H
