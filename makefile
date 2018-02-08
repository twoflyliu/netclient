SOURCES := $(wildcard *.c)
SOURCES := $(filter-out lua_downloader.c app.c,$(SOURCES)) #移除lua 扩展文件和一个测试整体使用的app.c文件
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

# 分别表示要生成的动态库和静态库
AR = libnetclient.a
SO = libnetclient.so
LUA_EXTENSION = nc.so
GLOB_TEST = app

# 指定lua的头文件路径和库文件路径
LUA_INCLUDE ?= /usr/include/lua5.3
LUA_LIB ?= /usr/local/lib

release ?= 0

TEST_FILES = test_util test_list test_http_headers test_socket_openssl

# 为了方便起见，将lua的头文件直接添加到CFLAGS中
ifeq (0,$(release))
	CFLAGS = -Wall -g -I$(LUA_INCLUDE)
else
	CFLAGS = -Wall -I$(LUA_INCLUDE)
endif

CFLAGS += -fPIC

LOADLIBS = -lssl -lcrypto

all: $(AR) $(SO) app nc.so

$(AR): $(OBJECTS)
	ar rcs $@ $^

$(SO): $(OBJECTS)
	gcc -shared -fPIC -o $@ $^ $(LOADLIBS)

# 生成netclient整体的测试app(这儿附带测试动态库)
# TODO: 或许应该创建一个netclient前端，类似curl东西，还是先把必要的协议给实现一下
$(GLOB_TEST): app.o $(AR)
	gcc -o $@ $^ $(LOADLIBS) $(LDLIBS)

# 生成netclient的lua扩展(这儿附带测试静态库, 静态库只能打包o文件，打包不了o文件的依赖关系)
$(LUA_EXTENSION): lua_downloader.o $(AR)
	gcc -shared -fPIC -o $@ $^ $(LOADLIBS) -L$(LUA_LIB)  -llua5.3

# 因为有些文件有其他依赖，可以用工具生成，或者手写
test_http_headers: list.o

# 生成测试，运行测试
test: test_exe_files = $(patsubst %,./%;,$^)
test: $(TEST_FILES)
	$(test_exe_files)

test_%: %_t.o
	gcc -o $@ $^ $(LOADLIBS)

# 测试版本匹配规则
%_t.o: CFLAGS = -DTEST_$(shell echo $(patsubst %.c,%,$<) | tr a-z A-Z) -g -Wall
%_t.o: %.c
	gcc $(CFLAGS) -c -o $@ $^

.PHONY: test clean debug

clean:
	$(RM) *.o  *.exe

debug:
	@echo $(filter-out lua_downloader.c,$(SOURCES))
	echo $(SOURCES)
