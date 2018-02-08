/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 06 13:24:35
*/
#include <stdlib.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

#include "util.h"
#include "socket.h"


#define SSL_ERROR() ERR_reason_error_string(ERR_get_error())

#define ERROR(fail, fmt, ...) \
    if (!(fail)) { \
        fprintf(stderr, fmt, ##__VA_ARGS__);\
        exit(1);\
    }

struct _Socket {
    int ssl;
    BIO *bio;
};

static SSL_CTX *ssl_ctx;

// 利用两个gcc扩展，constructor里面的括号表示优先级
// 值越小越先执行
__attribute__((constructor(101)))
static void openssl_setup()
{
    SSL_library_init(); // 必须要调用
    
    ssl_ctx = SSL_CTX_new(SSLv23_client_method());
    ERROR(ssl_ctx != NULL, "ssl ctx new failed!: %s\n", SSL_ERROR());

    // 加载证书位置，自己选择性设置
    /*if (!SSL_CTX_load_verify_locations(ssl_ctx, "/usr/ssl/cert.pem", NULL)) {*/
        /*ERROR(1, "ssl ctx new failed!: %s\n", SSL_ERROR());*/
    /*}*/
}

__attribute__((destructor(101)))
static void openssl_teardown()
{
    if (ssl_ctx) {
        SSL_CTX_free(ssl_ctx);
    }
}

Socket *socket_create(int enable_ssl)
{
    Socket *thiz = (Socket*)malloc(sizeof(Socket));
    if (thiz != NULL) {
        thiz->ssl = enable_ssl;
        thiz->bio = NULL;
    }
    return thiz;
}

void socket_destroy(Socket *thiz)
{
    if (thiz->bio != NULL) {
        BIO_free_all(thiz->bio);
        thiz->bio = NULL;
    }
    free(thiz);
}

static inline void socket_new_bio(Socket *thiz, const char *hostname, unsigned short port)
{
    char buf[BUFSIZ];
    SSL *ssl = NULL;
    int n;
    return_if_fail(thiz != NULL && thiz->bio == NULL);
    n = snprintf(buf, sizeof(buf) - 1, "%s:%d", hostname, port);
    buf[n] = '\0';
    if (thiz->ssl) {
        thiz->bio = BIO_new_ssl_connect(ssl_ctx);
        ERROR(thiz->bio != NULL, "bio new ssl failed: %s, host: %s\n", SSL_ERROR(), buf);
        BIO_get_ssl(thiz->bio, &ssl);
        ERROR(ssl != NULL, "ssl new failed!: %s, host: %s\n", SSL_ERROR(), buf);
        SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
        BIO_set_conn_hostname(thiz->bio, buf);
    } else {
        thiz->bio = BIO_new_connect(buf);
        ERROR(thiz->bio != NULL, "bio new ssl failed: %s, host: %s\n", SSL_ERROR(), buf);
    }
    BIO_set_nbio(thiz->bio, 1); //设置套接字为异步模式
}

/*!
 * \brief socket_connect 连接服务器
 * \param [in,out] thiz Socket对象
 * \param [in] host 主机名称
 * \param [in] port 主机端口号
 * \retval <=0 表示出错，> 0 表示成功
 */
int socket_connect(Socket *thiz, const char *hostname, unsigned short port)
{
    return_value_if_fail(thiz != NULL && hostname != NULL && *hostname != '\0', -1);
    if (NULL == thiz->bio) socket_new_bio(thiz, hostname, port);
    return BIO_do_connect(thiz->bio);
}

/*!
 *  \brief socket_should_retry 检测是否是因为非阻塞操作而出错
 *  \param [in] thiz Socket对象
 *  \retval 1 表示是的，否则表示不是
 */
int socket_should_retry(Socket *thiz)
{
    return_value_if_fail(thiz != NULL, 0);
    if (NULL == thiz->bio) return 0;
    return BIO_should_retry(thiz->bio);
}

/*!
 *  \brief socket_error 返回具体的出错原因
 */
const char *socket_error(Socket *thiz)
{
    return_value_if_fail(thiz != NULL, "");
    return SSL_ERROR();
}

/*!
 *  \brief socket_write 输出数据到服务器
 *  \param [in,out] thiz
 *  \param [in] buf 要输出的内容
 *  \param [in] buflen 要输出的长度
 *  \retval <0 表示出错，=0 表示服务器关闭，>0 表示输出的字节数 
 */
int socket_write(Socket *thiz, void *buf, int buflen)
{
    return_value_if_fail(thiz != NULL && buf != NULL && buflen > 0, -1);
    return BIO_write(thiz->bio, buf, buflen);
}

/*!
 *  \brief socket_read 从服务器上接收数据
 *  \param [in,out] thiz
 *  \param [in] buf 要输出的内容
 *  \param [in] buflen 要输出的长度
 *  \retval <0 表示出错，=0 表示服务器关闭，>0 表示输出的字节数目
 */
int socket_read(Socket *thiz, void *buf, int buflen)
{
    return_value_if_fail(thiz != NULL && buf != NULL && buflen > 0, -1);
    return BIO_read(thiz->bio, buf, buflen);
}

int socket_get_fd(Socket *thiz)
{
    return_value_if_fail(thiz != NULL, -1);
    if (NULL == thiz->bio) {
        return -1;
    }
    return BIO_get_fd(thiz->bio, NULL);
}

#ifdef TEST_SOCKET_OPENSSL

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

void test_create_destroy()
{
    Socket *socket = socket_create(1);
    assert(socket != NULL);
    socket_destroy(socket);
}

void test_get_fd()
{
    Socket *socket = socket_create(1);
    assert(-1 == socket_get_fd(socket));
    socket_connect(socket, "www.37zw.net", 80);
    assert(socket_get_fd(socket) >= 0);
    socket_destroy(socket);
}

void test_normal_use()
{
    char buf[BUFSIZ] = {"GET / HTTP/1.0\r\nHost: www.ifeng.net\r\n\r\n"};
    const char *host = "www.ifeng.com";
    Socket *socket = socket_create(0);
    int index = 0;
    int buflen = strlen(buf);
    int ret = 0;

    assert(socket != NULL);

    // 连接服务器, 因为是异步，所以一直等待成功
    while (socket_connect(socket, host, 80) <= 0) {
        assert(socket_should_retry(socket));
        usleep(1000 * 100); //睡眠100ms 防止拖累cpu
    }

    // 发送数据
    for (;;) {
        ret = socket_write(socket, buf + index, buflen - index);
        if (ret <= 0) {
            assert(socket_should_retry(socket));
            usleep(1000 * 100); //睡眠100ms 防止拖累cpu
        } else {
            index += ret;
            if (index == buflen) break; // 这儿这样设计防止数据没有一次性发送过去
        }
    }

    // 接收数据
    for (;;) {
        ret = socket_read(socket, buf, sizeof(buf));
        if (ret < 0) { // 失败
            assert(socket_should_retry(socket));
            usleep(1000 * 100); //睡眠100ms 防止拖累cpu
        } else  if(0 == ret) { // 表示连接被关闭
            break;
        } else { // 表示成功

        }
    }
    assert(socket_get_fd(socket) > -1); 
    socket_destroy(socket);
}

int main(void)
{
    test_create_destroy();
    test_get_fd();
    test_normal_use();
    return 0;
}

#endif
