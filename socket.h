#ifndef SOCKET_H_H
#define SOCKET_H_H

/*!
 * \brief 非阻塞套接字，并且支持ssl
 */
struct _Socket;
typedef struct _Socket Socket;

Socket *socket_create(int enable_ssl);
void socket_destroy(Socket *thiz);

/*!
 * \brief socket_connect 连接服务器
 * \param [in,out] thiz Socket对象
 * \param [in] host 主机名称
 * \param [in] port 主机端口号
 * \retval <=0 表示出错，> 0 表示成功
 */
int socket_connect(Socket *thiz, const char *hostname, unsigned short port);

/*!
 *  \brief socket_should_retry 检测是否是因为非阻塞操作而出错
 *  \param [in] thiz Socket对象
 *  \retval 1 表示是的，否则表示不是
 */
int socket_should_retry(Socket *thiz);

/*!
 *  \brief socket_error 返回具体的出错原因
 */
const char *socket_error(Socket *thiz);

/*!
 *  \brief socket_write 输出数据到服务器
 *  \param [in,out] thiz
 *  \param [in] buf 要输出的内容
 *  \param [in] buflen 要输出的长度
 *  \retval <0 表示出错，=0 表示服务器关闭，>0 表示输出的字节数 
 */
int socket_write(Socket *thiz, void *buf, int buflen);

/*!
 *  \brief socket_read 从服务器上接收数据
 *  \param [in,out] thiz
 *  \param [in] buf 要输出的内容
 *  \param [in] buflen 要输出的长度
 *  \retval <0 表示出错，=0 表示服务器关闭，>0 表示输出的字节数目
 */
int socket_read(Socket *thiz, void *buf, int buflen);

/*!
 *  \brief socket_getfd 获取内部绑定的文件描述符
 */
int socket_get_fd(Socket *thiz);
#endif //SOCKET_H_H
