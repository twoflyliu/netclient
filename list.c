/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 05 18:15:57
*/
#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "list.h"


typedef struct _Node {
    void *data;
    struct _Node *prev;
    struct _Node *next;
} Node;

static Iterator *list_iterator_create(List *list, Node *node);
static Iterator *list_reverse_iterator_create(List *list, Node *node);

struct _List {
    Node *head, *tail;
    size_t len;
    DataDestroyFunc data_destroy;
};

static Node* list_find_node(List *thiz, void *data);

static inline Node* node_create(void *data)
{
    Node *thiz = (Node*)calloc(sizeof(Node), 1);
    if (thiz != NULL) {
        thiz->data = data;
    }
    return thiz;
}

static inline void list_node_destroy(List *thiz, Node *node)
{
    if (thiz->data_destroy) thiz->data_destroy(node->data);
    free(node);
}

static void list_node_erase(List *thiz, Node *node)
{
    return_if_fail(thiz != NULL && node != NULL);
    if (node == thiz->head) {
        list_pop_front(thiz);
    } else if (node == thiz->tail) {
        list_pop_back(thiz);
    } else {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        list_node_destroy(thiz, node);
        --thiz->len;
    }
}

List* list_create(DataDestroyFunc data_destroy)
{
    List *thiz = (List*)calloc(sizeof(List), 1);
    if (thiz != NULL) {
        thiz->data_destroy = data_destroy;
    }
    return thiz;
}

void list_destroy(List *thiz)
{
    return_if_fail(thiz != NULL);
    Node *cur, *next;
    next = thiz->head;
    while (next != NULL) {
        cur = next;
        next = next->next;
        list_node_destroy(thiz, cur);
    }
    free(thiz);
}

void* list_get(List *thiz, size_t index)
{
    Node *node = NULL;
    return_value_if_fail(thiz != NULL && index < list_size(thiz), NULL);
    node = thiz->head;
    for (size_t i = 0; i < index; i++) {
        node = node->next;
    }
    return node->data;
}

void* list_front(List *thiz)
{
    return_value_if_fail(thiz != NULL && thiz->head != NULL, NULL);
    return thiz->head->data;
}

void* list_back(List *thiz)
{
    return_value_if_fail(thiz != NULL && thiz->tail != NULL, NULL);
    return thiz->tail->data;
}

void list_push_back(List *thiz, void *data)
{
    Node *new_node = NULL;
    return_if_fail(thiz != NULL);
    new_node = node_create(data);
    return_if_fail(new_node != NULL);
    if (thiz->tail == NULL) {
        thiz->head = thiz->tail = new_node;
    } else {
        thiz->tail->next = new_node;
        new_node->prev = thiz->tail;
        thiz->tail = new_node;
    }
    thiz->len ++;
}

void list_push_front(List *thiz, void *data)
{
    Node *new_node = NULL;
    return_if_fail(thiz != NULL);
    new_node = node_create(data);
    return_if_fail(new_node != NULL);
    if (thiz->head == NULL) {
        thiz->head = thiz->tail = new_node;
    } else {
        thiz->head->prev = new_node;
        new_node->next = thiz->head;
        thiz->head = new_node;
    }
    thiz->len++;
}

void list_pop_front(List *thiz)
{
    return_if_fail(thiz != NULL && thiz->head != NULL);
    if (thiz->head == thiz->tail) {
        list_node_destroy(thiz, thiz->head);
        thiz->head = thiz->tail = NULL;
    } else {
        Node *new_head = thiz->head->next;
        new_head->prev = NULL;
        list_node_destroy(thiz, thiz->head);
        thiz->head = new_head;
    }
    thiz->len --;
}

void list_pop_back(List *thiz)
{
    return_if_fail(thiz != NULL && thiz->tail != NULL);
    if (thiz->tail == thiz->head) {
        list_node_destroy(thiz, thiz->tail);
        thiz->tail = thiz->head = NULL;
    } else {
        Node *new_tail = thiz->tail->prev;
        new_tail->next = NULL;
        list_node_destroy(thiz, thiz->tail);
        thiz->tail = new_tail;
    }
    thiz->len --;
}

size_t list_size(List *thiz)
{
    return_value_if_fail(thiz != NULL, 0);
    return thiz->len;
}

static Node* list_find_node(List *thiz, void *data)
{
    Node *cursor = thiz->head;
    while (cursor != NULL) {
        if (cursor->data == data) break;
        cursor = cursor->next;
    }
    return cursor;
}

void list_erase(List *thiz, void *data)
{
    Node *node = NULL;
    return_if_fail(thiz != NULL && data != NULL);
    node = list_find_node(thiz, data);
    return_if_fail(node != NULL);
    list_node_erase(thiz, node);
}

void* list_find(List *thiz, DataCompareFunc cmp, void *ctx)
{
    Node *node = NULL;
    return_value_if_fail(thiz != NULL && cmp != NULL && ctx != NULL, NULL);
    for (node = thiz->head; node != NULL; node = node->next) {
        if (cmp(ctx, node->data) == 0) break;
    }
    return (node != NULL) ? node->data : NULL;
}

Iterator* list_begin(List *thiz)
{
    return_value_if_fail(thiz != NULL && thiz->head != NULL, NULL);
    return list_iterator_create(thiz, thiz->head);
}

Iterator* list_rbegin(List *thiz)
{
    return_value_if_fail(thiz != NULL && thiz->tail != NULL, NULL);
    return list_reverse_iterator_create(thiz, thiz->tail);
}

typedef struct _IteratorPriv  {
    List *list;
    Node *node;
} IteratorPriv;

static void* list_iterator_data(Iterator *thiz)
{
    IteratorPriv *priv = NULL;
    return_value_if_fail(thiz != NULL, NULL);
    priv = (IteratorPriv*)thiz->priv;
    return_value_if_fail(priv->node != NULL, NULL);
    return priv->node->data;
}

static void list_iterator_next(Iterator *thiz)
{
    IteratorPriv *priv;
    return_if_fail(thiz != NULL);
    priv = (IteratorPriv*)thiz->priv;
    return_if_fail(priv->node != NULL);
    priv->node = priv->node->next;
}

static void list_iterator_prev(Iterator *thiz)
{
    IteratorPriv *priv;
    return_if_fail(thiz != NULL);
    priv = (IteratorPriv*)thiz->priv;
    return_if_fail(priv->node != NULL);
    priv->node = priv->node->prev;
}

static int list_iterator_is_done(Iterator *thiz)
{
    IteratorPriv *priv;
    return_value_if_fail(thiz != NULL, 1);
    priv = (IteratorPriv*)thiz->priv;
    return priv->node == NULL;
}

static void list_iterator_erase(Iterator *thiz)
{
    IteratorPriv *priv;
    Node *new_cur;
    return_if_fail(thiz != NULL);
    priv = (IteratorPriv*)thiz->priv;
    new_cur = priv->node->next;
    list_node_erase(priv->list, priv->node);
    priv->node = new_cur; // 删除后原来后面的节点就是新的节点
}

void list_iterator_destroy(Iterator *thiz);

static Iterator *list_iterator_create(List *list, Node *node)
{
    Iterator *thiz = NULL;
    return_value_if_fail(node != NULL, NULL);
    if ((thiz = (Iterator*)malloc(sizeof(Iterator) + sizeof(IteratorPriv))) != NULL) {
        IteratorPriv *priv = (IteratorPriv*)thiz->priv;
        thiz->data = list_iterator_data;
        thiz->next = list_iterator_next;
        thiz->is_done = list_iterator_is_done;
        thiz->destroy = list_iterator_destroy;
        thiz->erase = list_iterator_erase;
        priv->node = node;
        priv->list = list;
    }
    return thiz;
}

static Iterator *list_reverse_iterator_create(List *list, Node *node)
{
    Iterator *thiz = NULL;
    return_value_if_fail(node != NULL, NULL);
    if ((thiz = (Iterator*)malloc(sizeof(Iterator) + sizeof(IteratorPriv))) != NULL) {
        IteratorPriv *priv = (IteratorPriv*)thiz->priv;
        thiz->data = list_iterator_data;
        thiz->next = list_iterator_prev;
        thiz->is_done = list_iterator_is_done;
        thiz->erase = list_iterator_erase;
        thiz->destroy = list_iterator_destroy;
        priv->node = node;
        priv->list = list;
    }
    return thiz;
}

void list_iterator_destroy(Iterator *thiz)
{
    free(thiz);
}

#ifdef TEST_LIST

#ifdef NDEBUG
#undef NDEBUG
#endif

static void test_base()
{
    List *list = list_create(NULL);
    const size_t test_count = 1000;
    size_t len;

    // 检测push_back的正确定性
    for (size_t i = 0; i < test_count; i++) {
        list_push_back(list, (void*)i);
        assert(i+1 == list_size(list));
    }
    for (size_t i = 0; i < test_count; i++) {
        assert(i == (size_t)list_get(list, i));
    }

    len = list_size(list);
    assert(test_count == len);

    // 检测front和pop_front的正确性
    for (size_t i = 0; i < test_count; i++) {
        assert(i == (size_t)list_front(list));
        list_pop_front(list);
        -- len;
        assert(len == list_size(list));
    }

    assert(0 == list_size(list));

    // 检测push_front的正确性
    for (size_t i = 0; i < test_count; i++) {
        list_push_front(list, (void*)i);
        assert(i+1 == list_size(list));
    }
    for (size_t i = 0; i < test_count; i++) {
        assert(i == (size_t)list_get(list, test_count - 1 - i));
    }

    len = list_size(list);
    assert(len == test_count);

    // 检测back和pop_back的正确性
    for (size_t i = 0; i < test_count; i++) {
        assert(i == (size_t)list_back(list));
        list_pop_back(list);
        -- len;
        assert(len == list_size(list));
    }
    assert(0 == list_size(list));

    list_destroy(list);
}

static void test_iterator()
{
    List *list = list_create(NULL);
    const size_t test_count = 1000;
    Iterator *iter = NULL;
    size_t i;
    
    for (i = 0; i < test_count; i++) {
        list_push_back(list, (void*)i);
    }
    assert(test_count == list_size(list));

    i = 0;
    for (iter = list_begin(list); !iterator_is_done(iter); iterator_next(iter)) {
        assert(i == (size_t)iterator_data(iter));
        i++;
    }
    iterator_destroy(iter);

    i = test_count - 1;
    for (iter = list_rbegin(list); !iterator_is_done(iter); iterator_next(iter)) {
        assert(i == (size_t)iterator_data(iter));
        --i;
    }
    iterator_destroy(iter);

    i = 0;
    for (iter = list_begin(list); !iterator_is_done(iter); ) {
        assert(i == (size_t)iterator_data(iter));
        i++;
        iterator_erase(iter); // 删除元素，不使用向后推进
    }
    iterator_destroy(iter);
    
    assert(0 == list_size(list));
    list_destroy(list);
}

int main(void)
{
    test_base();
    test_iterator();
    return 0;
}

#endif
