#ifndef SELECTOR_H_H
#define SELECTOR_H_H

struct _Selector;
typedef struct _Selector Selector;

Selector* selector_create();
void selector_destroy(Selector *thiz);

void selector_add_read(Selector *thiz, int fd);     //!< 添加监控读
void selector_add_write(Selector *thiz, int fd);    //!< 添加些监控

void selector_rm_read(Selector *thiz, int fd);
void selector_rm_write(Selector *thiz, int fd);

int selector_can_read(Selector *thiz, int fd);      //!< 测试fd是否可读
int selector_can_write(Selector *thiz, int fd);     //!< 测试fd是否可写

int selector_max_fd(Selector *thiz); //!< 返回最多可以监控的套接字数目
void selector_select(Selector *thiz, int millsec); //进行堵路复用

#endif //SELECTOR_H_H
