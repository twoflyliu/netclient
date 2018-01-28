#!/usr/bin/env lua
local nc = require("nc").new("error") --里面参数是调试级别
local logging = require("logging")

local req = {
    url="http://www.37zw.net/",
    headers={
        ["User-Agent"]="Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/62.0.3202.62 Safari/537.36",
        Host="www.37ze.net",
    },
    method="GET",
}

nc:download(req, function(b, c, h, s)
    if b == nil then
        print('error: ' .. c)
    else
        print('================================================>')
        --print(b)
        print('url: ' .. req.url)
        print('content-length: ' .. #b)
        print('headers: ' .. logging.tostring(h))
        print('code: ' .. c)
        print('status: ' .. s)
        print('<===============================================')
    end
end)

nc:download("https://www.jianshu.com/p/4f9e79eb0163", function(b, c, h, s)
    if b == nil then
        print('error: ' .. c)
    else
        print('================================================>')
        --print(b)
        print('url: ' .. "https://www.jianshu.com/p/4f9e79eb0163")
        print('content-length: ' .. #b)
        print('headers: ' .. logging.tostring(h))
        print('code: ' .. c)
        print('status: ' .. s)
        print('<===============================================')
    end
end)

-- 一般提交完表单会重定向
-- 模拟POST请求
-- 下面的127.0.0.1:9090是自己内地测试服务器
--nc:download({url="http://127.0.0.1:9090/login", 
    ----headers={
        ----['Content-Length']="17", 
        ----['Content-Type']="application/x-www-form-urlencoded",
    ----},
    --data="uname=ffx&pwd=12k", 
    --method="POST"}, function(b, c, h, s)
        --print('====================POST=======================')
        --print('body:' .. b)
        --print('code:' .. c)
        --print(logging.tostring(h))
        --print('status:' .. s)
--end)

---- 模拟HEAD请求
---- 支持同个url，不同请求方法的绑定
--nc:download({url="http://127.0.0.1:9090/login", 
    --method="HEAD"}, function(b, c, h, s)
        --print('================HEAD===========================')
        --print('body:' .. b) --Content-Length显示为10，但是为空，因为HEAD请求的特性
        --print('code:' .. c)
        --print(logging.tostring(h))
        --print('status:' .. s)
--end)


nc:join()
