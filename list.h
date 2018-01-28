#ifndef LIST_H_H
#define LIST_H_H

#include <stddef.h>
#include "util.h"

struct _List;
typedef struct _List List;

struct _Iterator;
typedef struct _Iterator Iterator;

// 迭代器接口
struct _Iterator {
    void (*next)(Iterator *thiz);
    int (*is_done)(Iterator *thiz);
    void (*destroy)(Iterator *thiz);
    void (*erase)(Iterator *thiz);
    void* (*data)(Iterator *thiz);
    char priv[0];
};

static inline void iterator_next(Iterator *thiz)
{
    return_if_fail(thiz != NULL);
    thiz->next(thiz);
}

static inline int iterator_is_done(Iterator *thiz)
{
    return_value_if_fail(thiz != NULL, 1);
    return thiz->is_done(thiz);
}

static inline void iterator_destroy(Iterator *thiz)
{
    return_if_fail(thiz != NULL);
    return thiz->destroy(thiz);
}

static inline void iterator_erase(Iterator *thiz)
{
    return_if_fail(thiz != NULL);
    thiz->erase(thiz);
}

static inline void* iterator_data(Iterator *thiz)
{
    return_value_if_fail(thiz != NULL, NULL);
    return thiz->data(thiz);
}

typedef void (*DataDestroyFunc)(void *ctx);

/*!
 *  \brief DataCompareFunc 比较两个值
 *  \retval =0 表示相等， <0 表示 *ctx < *ele, >0 表示 *ctx > *ele
 */
typedef int (*DataCompareFunc)(void *ctx, void *ele);

List* list_create(DataDestroyFunc data_destroy);
void list_destroy(List *thiz);

void* list_get(List *thiz, size_t index);
void* list_front(List *thiz);
void* list_back(List *thiz);
int list_empty(List *thiz); //检测list是否为空

void list_push_back(List *thiz, void *data);
void list_push_front(List *thiz, void *data);


void list_pop_front(List *thiz);
void list_pop_back(List *thiz);

void list_erase(List *thiz, void *data);
void* list_find(List *thiz, DataCompareFunc cmp, void *ctx);

size_t list_size(List *thiz);

Iterator* list_begin(List *thiz);
Iterator* list_rbegin(List *thiz);

void iterator_destroy(Iterator *thiz);

#endif //LIST_H_H
