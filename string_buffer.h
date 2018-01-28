#ifndef STRING_BUFFER_H_H
#define STRING_BUFFER_H_H

#include <stddef.h>

/*!
 * \brief StringBuffer内部不但可以保存字符串，内部还可以保存二进制数据
 * 
 * 所以里面的内部就不能使用字符串来进行展示了
 * 
 * TODO: 或许可以将这个结构体直接命名为Buffer比较好，先暂时就先这样
 */
struct _StringBuffer;
typedef struct _StringBuffer StringBuffer;

StringBuffer *string_buffer_create(size_t init_capacity);
void string_buffer_destroy(StringBuffer *thiz);

// 内部也可以保存二进制数据
void string_buffer_append_data(StringBuffer *thiz, const void *data, size_t datalen);

void string_buffer_append(StringBuffer *thiz, const char *str);
char * string_buffer_c_str(StringBuffer *thiz);
void string_buffer_clear(StringBuffer *thiz);

size_t string_buffer_size(StringBuffer *thiz);
size_t string_buffer_capacity(StringBuffer *thiz);

#endif //STRING_BUFFER_H_H
