#ifndef STRING_BUFFER_H_H
#define STRING_BUFFER_H_H

#include <stddef.h>

struct _StringBuffer;
typedef struct _StringBuffer StringBuffer;

StringBuffer *string_buffer_create(size_t init_capacity);
void string_buffer_destroy(StringBuffer *thiz);

void string_buffer_append(StringBuffer *thiz, const char *str);
char * string_buffer_c_str(StringBuffer *thiz);
void string_buffer_clear(StringBuffer *thiz);

size_t string_buffer_size(StringBuffer *thiz);
size_t string_buffer_capacity(StringBuffer *thiz);

#endif //STRING_BUFFER_H_H
