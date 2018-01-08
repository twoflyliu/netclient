/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 07 20:14:46
*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "string_buffer.h"

#define DEFAULT_CAPACITY 10

struct _StringBuffer {
    char *buf;
    size_t len;
    size_t capacity;
};

StringBuffer *string_buffer_create(size_t init_capacity)
{
    StringBuffer *thiz = (StringBuffer*)malloc(sizeof(StringBuffer));
    if (thiz != NULL) {
        init_capacity = __max(DEFAULT_CAPACITY, init_capacity);
        if ((thiz->buf = (char*)malloc(init_capacity)) == NULL) {
            free(thiz);
            thiz = NULL;
        } else {
            thiz->len = 0;
            thiz->capacity = init_capacity;
            thiz->buf[0] = '\0';
        }
    }
    return thiz;
}

void string_buffer_destroy(StringBuffer *thiz)
{
    return_if_fail(thiz != NULL);
    if (thiz->buf) {
        free(thiz->buf);
        thiz->buf = NULL;
    }
    free(thiz);
}

static int _string_buffer_expand(StringBuffer *thiz, size_t need_space)
{
    if (thiz->len + need_space >= thiz->capacity) {
        size_t new_capacity = thiz->capacity + __max((thiz->capacity << 1), need_space + 1);
        char *new_buf = (char*)realloc(thiz->buf, new_capacity);
        if (new_buf != NULL) {
            thiz->buf = new_buf;
            thiz->capacity = new_capacity;
        } else {
            return_value_if_fail(new_buf != NULL, 0); // 表示没有可用内存了
        }
    }
    return 1;
}

void string_buffer_append(StringBuffer *thiz, const char *str)
{
    size_t str_len = 0;
    return_if_fail(thiz != NULL && str != NULL);
    str_len = strlen(str);
    if (_string_buffer_expand(thiz, str_len)) {
        strcat(thiz->buf, str);
        thiz->len += str_len;
    } else {
        assert(0 && "non memory usable");
    }
}

char * string_buffer_c_str(StringBuffer *thiz)
{
    return_value_if_fail(thiz != NULL, NULL);
    return thiz->buf;
}

void string_buffer_clear(StringBuffer *thiz)
{
    return_if_fail(thiz != NULL);
    thiz->len = 0;
    thiz->buf[0] = '\0';
}

size_t string_buffer_size(StringBuffer *thiz)
{
    return_value_if_fail(thiz != NULL, 0);
    return thiz->len;
}

size_t string_buffer_capacity(StringBuffer *thiz)
{
    return_value_if_fail(thiz != NULL, 0);
    return thiz->capacity;
}

#ifdef TEST_STRING_BUFFER

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <string.h>

int main(void)
{
    StringBuffer *buf = string_buffer_create(5);
    assert(0 == strlen(string_buffer_c_str(buf)));
    assert(0 == string_buffer_size(buf));
    assert(DEFAULT_CAPACITY == string_buffer_capacity(buf));

    string_buffer_append(buf, "abc");
    assert(0 == strcmp("abc", string_buffer_c_str(buf)));

    string_buffer_append(buf, "hello world xxxxxxhahhahahahah");
    assert(0 == strcmp("abchello world xxxxxxhahhahahahah", string_buffer_c_str(buf)));
    assert(string_buffer_capacity(buf) > DEFAULT_CAPACITY);

    string_buffer_clear(buf);
    assert(0 == strlen(string_buffer_c_str(buf)));
    assert(0 == string_buffer_size(buf));

    string_buffer_destroy(buf);
    return 0;
}

#endif
