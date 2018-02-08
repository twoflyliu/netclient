/********************************************************
* Description: @description@
* Author: twoflyliu
* Mail: twoflyliu@163.com
* Create time: 2018 01 12 18:58:20
*/
#include "util.h"

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "net_client.h"
#include "http_client.h"

#include "response_listener.h"

#include "logger.h"
#include "logger_listener.h"

#include "request.h"

#define NC_VERSION "v0.0.1"
#define NC_REG_KEY "ffx.netclient"

// 这儿使用一个表主要解决不同协议
// 不同请求参数问题，使用表首先方便维护，其次方便简化问题(分而治之的思想)
typedef Request* (*RequestCreator)(lua_State *L, const char *url); //url是用来接收url值
typedef struct _RequestCreatorEntry {
    const char *protocol;
    RequestCreator creator;
} RequestCreatorEntry;

static Request* http_request_creator(lua_State *L, const char *url);
static Request* ftp_request_creator(lua_State *L, const char *url);

RequestCreatorEntry request_creators[] = {
    {"http", http_request_creator},
    {"https", http_request_creator},
    {"ftp", ftp_request_creator},
    {NULL, NULL}
};

/*!
 *  \brief request_create 用来将lua字符串或者table转换为c对象
 */
static Request* request_create(lua_State *L, char url[], size_t url_size)
{
    static const char *default_protocol = "http";
    const char *protocol = default_protocol;
    size_t protocol_len = 4; //默认是http长度
    RequestCreatorEntry *entry = NULL;
    const char *p = NULL;

    // 首先提取url
    if (LUA_TSTRING == lua_type(L, 2)) {
        strncpy(url, lua_tostring(L, 2), url_size);
    } else if (LUA_TTABLE == lua_type(L, 2)) {
        lua_pushvalue(L, 2);
        lua_getfield(L, -1, "url");
        luaL_argcheck(L, lua_isstring(L, -1), 2, "'request' table should contains url field");
        strncpy(url, lua_tostring(L, -1), url_size);
        lua_pop(L, 2); //恢复原来的栈结构
    }

    // 然后根据协议类型，使用具体的creator来进行创建
    if ((p = strchr(url, ':')) != NULL) {
        protocol = url;
        protocol_len = p - url;
    }

    entry = request_creators;
    while (entry->protocol != NULL) {
        if (0 == strncmp(protocol, entry->protocol, protocol_len)) {
            return entry->creator(L, url);
        }
        ++ entry;
    }

    return NULL;
}

static inline void http_request_method_init(lua_State *L, HttpRequest *req)
{
    int type = -1;
    lua_getfield(L, -1, "method");
    if ((type = lua_type(L, -1)) != LUA_TNIL) {
        int method = HTTP_REQUEST_METHOD_GET;
        luaL_argcheck(L, LUA_TSTRING == type, 2, "request table's method field should be string type and it can be  GET or POST or HEAD");
        method  = http_request_method_from_string(lua_tostring(L, -1));
        luaL_argcheck(L, method >= 0, 2, "request table's method field should be GET or POST or HEAD");
        req->method = method;
    }
    lua_pop(L, 1); //恢复栈结构
}

static inline void http_request_data_init(lua_State *L, HttpRequest *req)
{
    int type = -1;
    lua_getfield(L, -1, "data");
    if ((type = lua_type(L, -1)) != LUA_TNIL) {
        luaL_argcheck(L, LUA_TSTRING == type, 2, "request table's data field should be string type");
        req->data = lua_tolstring(L, -1, &req->datalen);
    }
    lua_pop(L, 1);
}

static inline void http_request_headers_init(lua_State *L, HttpRequest *req)
{
    int type = -1;
    lua_getfield(L, -1, "headers");
    if ((type = lua_type(L, -1)) != LUA_TNIL) {
        luaL_argcheck(L, LUA_TTABLE == type, 2, "request table's headers field should be table type");
        req->headers_type = HTTP_HEADERS_TYPE_MAP;
        req->map_headers = http_headers_create();
        // 遍历table
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {//弹出key，获取新的key, value
            //-2是key, -1是value
            luaL_argcheck(L, LUA_TSTRING == lua_type(L, -2), 2, "request.headers's key should be string");
            luaL_argcheck(L, LUA_TSTRING == lua_type(L, -2), 2, "request.headers's value should be string");
            http_headers_add(req->map_headers, lua_tostring(L, -2), 
                    lua_tostring(L, -1));
            lua_pop(L, 1); //弹出value，保证-1是key
        }
    }
    lua_pop(L, 1); //req.headers从栈上移除
}

static Request* http_request_creator(lua_State *L, const char *url)
{
    static HttpRequest request; //这样写性能没有损耗(确定不是线程安全的)

    request.base.protocol = PROTOCOL_HTTP;
    request.base.url = url;
    request.method = HTTP_REQUEST_METHOD_GET;
    request.data = NULL;
    request.datalen = 0;
    request.headers_type = HTTP_HEADERS_TYPE_NONE;

    if (LUA_TTABLE == lua_type(L, 2)) {
        lua_pushvalue(L, 2);
        http_request_method_init(L, &request); // 初始化method
        http_request_data_init(L, &request); // 初始化data
        http_request_headers_init(L, &request); // 初始化headers
        lua_pop(L, 1); //把最后的table也给移除掉
    }

    // 因为这个request是被添加到任务里面，所以内部如果使用HttpHeaders来保存头的话
    // request内部的map_headers会被直接移动到HttpClient中进行维护，所以
    // 这个时候，也不会存在内存泄露，所以也暂时不用讲Request抽象成一个接口，等到实在需要的时候，在进行抽象
    return (Request*)&request;
}

static Request* ftp_request_creator(lua_State UNUSED *L, const char UNUSED *url)
{
    return NULL;
}



/*!
 * \brief Client 每个客户端要关联的数据
 */
typedef struct _NC_Client {
    NetClient *client;     //!< 每个下载模块的核心，真正处理协议问题
    lua_State *L;
} NCClient;

static inline void _lua_pushheaders(lua_State *L, HttpHeaders *headers)
{
    const char ** keys = NULL;
    const char *value = NULL;
    if (NULL == headers) return;

    lua_newtable(L);
    keys = http_headers_keys(headers);
    while (keys != NULL && *keys != NULL) {
        value = http_headers_get(headers, *keys);
        lua_pushstring(L, value);
        lua_setfield(L, -2, *keys);
        ++ keys;
    }
}

static const char *_key_of_callback_from_response(Response *response, char buf[], size_t buflen);

/*!
 *  处理http响应
 */
static void _http_response_handler(void *data, Response *resp)
{
    NCClient *client = (NCClient*)data;
    HttpResponse *http_resp = (HttpResponse*)resp;
    lua_State *L = client->L;
    char buf[BUFSIZ];

    // 获取回调函数，进行调用就完毕了
    lua_pushlightuserdata(L, client);
    lua_gettable(L, LUA_REGISTRYINDEX); //获取对应的私有表
    lua_getfield(L, -1, _key_of_callback_from_response(resp, buf, sizeof(buf)));
    if (lua_isnil(L, -1)) {
        fprintf(stderr, "url '%s' miss binding callback\n", resp->url);
    } else {
        if (http_resp->base.success) {
            lua_pushstring(L, http_resp->response_content);
            lua_pushinteger(L, http_resp->code);
            _lua_pushheaders(L, http_resp->response_headers);
            lua_pushstring(L, http_resp->status);
            lua_call(L, 4, 0);
        } else {
            lua_pushnil(L);
            lua_pushstring(L, http_resp->error_msg);
            lua_call(L, 2, 0);
        }
    }
    lua_pop(L, 1); //移除私有table, lua_call会将函数和所有参数都弹出来
}

/*!
 *  处理ftp响应
 */
static void _ftp_response_handler(void UNUSED *data, Response UNUSED *resp)
{

}

// 还有多余的响应只需要在_net_client_add_response_handler中进行注册就行了
// 这样写主要为了方便扩展（因为是给lua提供api, 所以不设置成动态）
// 你只需要在_net_client_add_response_handler函数中调用response_listener_add_handler进行注册
// 对应的协议处理器就行了

/*!
 * \brief _logger_listener_create 根据栈上的第一个参数来决定创建什么样的日志监听器
 */
static inline EventListener * _logger_listener_create(lua_State *L)
{
    const char *logger_level = NULL;
    int level = -1;

    if (0 == lua_gettop(L)) {
        return NULL;
    }

    logger_level = luaL_checkstring(L, 1);
    if ((level = logger_level_from_string(logger_level)) >= 0) { // 将字符串级别转换为数字级别
        return logger_listener_create(level, NULL);
    }
    return NULL;
}

static inline void _net_client_add_response_handlers(NCClient *client, EventListener *response_listener)
{
    //NOTE: 如果想添加其他协议，只需要在下面进行注册对应的回调函数就行了
    response_listener_add_handler(response_listener, PROTOCOL_HTTP, client, _http_response_handler);
    response_listener_add_handler(response_listener, PROTOCOL_FTP, client, _ftp_response_handler);
}

static inline void _net_client_init(NCClient *client, EventListener *logger)
{
    NetClient *cli = client->client;
    EventListener *response_listener = response_listener_create(); 
    return_if_fail(response_listener != NULL); // 一般是不可能失败的

    // 注册http协议
    net_client_register_protocol(cli, PROTOCOL_HTTP,
            http_protocol_client_create, NULL);

    // 注册两个监听器，一个是日志监听器，一个是响应监听器
    // 对于日志，不关心具体协议类型，他监听所有的协议类型
    if (logger != NULL) {
        net_client_add_event_listener(cli, PROTOCOL_ALL, logger);
    }
            
    // 暂时不添加，通用response要重新设置接收才好用
    net_client_add_event_listener(cli, PROTOCOL_HTTP,
            response_listener);

    _net_client_add_response_handlers(client, response_listener);
}

// 检测第一个参数元表是否是指定元表
static inline NCClient* _check_client(lua_State *L) 
{
    return (NCClient*)luaL_checkudata(L, 1, NC_REG_KEY);
}

/*!
 * \brief 根据request类型来生成回调函数在私有table中的绑定key
 * 
 * 算法是：如果是http请求，那么就生成url@method组合字符串作为key
 * 否则就是原始的url
 * 
 * 一般url不会超过BUFSIZ
 */
static const char *_key_of_callback_from_request(Request *request, char buf[], size_t buflen)
{
    assert(buf != NULL && buflen > 0);
    if (PROTOCOL_HTTP == request->protocol) {
        int n = snprintf(buf, buflen, "%s@%d", request->url, ((HttpRequest*)request)->method);
        buf[__min(n, (int)buflen - 1)] = '\0';
        return buf;
    } else {
        return request->url;
    }
}

static const char *_key_of_callback_from_response(Response *response, char buf[], size_t buflen)
{
    assert(buf != NULL && buflen > 0);
    if (PROTOCOL_HTTP == response->protocol) {
        int n = snprintf(buf, buflen, "%s@%d", response->url, ((HttpResponse*)response)->request_method);
        buf[__min(n, (int)buflen - 1)] = '\0';
        return buf;
        
    } else {
        return response->url;
    }
}

/*!
 * \brief l_nc_download 添加一个下载任务
 * 
 * 用法：
 *   client.download("url", callback) -- 添加任务
 *   client.download("url", callback)
 *   
 *   client.download({url=>"具体url", data=>"请求数据", method=>"GET/POST", headers=>{}}, callback)
 *   
 *   client.join() -- 下载所有内容
 */
static int l_nc_download(lua_State *L)
{
    NCClient *client = _check_client(L);
    Request *request = NULL;
    char url[BUFSIZ] = {0};
    char key[BUFSIZ] = {0};

    luaL_argcheck(L, LUA_TTABLE == lua_type(L, 2) 
            || LUA_TSTRING == lua_type(L, 2), 2, "'table' or 'string' expected");
    luaL_checktype(L, 3, LUA_TFUNCTION);

    request = request_create(L, url, sizeof(url));
    return_value_if_fail(NULL != request, 0);

    net_client_push_request(client->client, request); //添加任务

    // 要在私有table里面放置url和回调函数的映射
    lua_pushlightuserdata(L, client);
    lua_gettable(L, LUA_REGISTRYINDEX);
    assert(LUA_TTABLE == lua_type(L, -1)); //先获取私有表
    lua_pushvalue(L, -2); //回调函数

    // linux不能把url，当做缓冲区，windows msys2上检测是行的，解决使用单独的缓冲区来构成key
    lua_setfield(L, -2, _key_of_callback_from_request(request, key, sizeof(key))); //url 只是相当于一个缓冲区
    lua_pop(L, 1);  //还原栈结构
    return 0;
}

static int l_nc_join(lua_State *L)
{
    NCClient *client = (NCClient*)_check_client(L);
    net_client_do_all(client->client); // 进行所有任务
    return 0;
}

/*!
 * \brief 对象级别析构函数
 */
static int l_nc_gc(lua_State *L)
{
    NCClient *client = (NCClient*)_check_client(L);

    lua_pushlightuserdata(L, client); //移除在保存在注册表中的数据
    lua_pushnil(L);
    lua_settable(L, LUA_REGISTRYINDEX);

    net_client_destroy(client->client); //销毁本地NetClient中所占用的资源
    return 0;
}

static const struct luaL_Reg meta_funcs[] = {
    {"download", l_nc_download},
    {"join", l_nc_join},
    {"__gc", l_nc_gc},
    {NULL, NULL}
};

static void _nc_metatable_init(lua_State *L)
{
    // 创建NetClient的元表
    luaL_newmetatable(L, NC_REG_KEY);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");  //设置__index属性是其自身
    luaL_setfuncs(L, meta_funcs, 0); //然后添加元表方法
    lua_pop(L, 1); //不改变栈上的结构
}

/*!
 * \brief l_new 新建一个netclient
 * 
 * 每个netclient都有自己的内部资源，他们之间的操作互不影响, 后面的注释表示在lua脚本中的参数说明
 * 
 * \param [in,opt] logger_level 表示输出日志级别，格式是字符串，可以是：
 * 
 * - none: 表示不输出任何调试信息
 * - fatal: 表示只输出致命级别调试信息
 * - error:
 *   warn:
 *   info:
 *   debug:
 */
static int l_nc_new(lua_State *L)
{
    EventListener *logger = _logger_listener_create(L);
    NCClient *client = (NCClient*)lua_newuserdata(L, sizeof(NCClient));
    return_value_if_fail(client != NULL, 0);

    client->client = net_client_create();
    return_value_if_fail(client->client != NULL, 0);

    // 注册常见协议，初始化对应的事件监听器
    _net_client_init(client, logger);

    client->L = L;

    // 为client注册元表
    luaL_getmetatable(L, NC_REG_KEY);
    lua_setmetatable(L, -2);

    // 也添加关联在这个数据的私有lua数据成员（因为不能直接在c中保存lua的函数和元表)
    lua_pushlightuserdata(L, client);
    lua_newtable(L);
    lua_settable(L, LUA_REGISTRYINDEX);
    return 1;
}

/*!
 *  \brief l_gc 模块级别销毁回调函数
 */
static int l_gc(lua_State UNUSED *L)
{
    return 0;
}


static const struct luaL_Reg mod_libs[] = {
    {"new", l_nc_new},
    {"__gc", l_gc},
    {NULL, NULL}
};

/*!
 * \brief luaopen_nc 整个绑定的入口函数
 * 
 * nc是netclient的简称，支持充当多种协议的客户端 
 */
int luaopen_nc(lua_State *L)
{
    _nc_metatable_init(L);

    // 创建直接导出表
    luaL_newlib(L, mod_libs);
    lua_pushvalue(L, -1); //设置元表是它的自身
    lua_setmetatable(L, -2);

    lua_pushstring(L, NC_VERSION); //设置模块版本属性
    lua_setfield(L, -2, "version");
    return 1;
}
