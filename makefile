SOURCES = $(wildcard *.c)
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

TEST_FILES = test_util test_list test_http_headers test_socket_openssl

CFLAGS = -Wall -g
LOADLIBS = -lssl -lcrypto

all: app

app: $(OBJECTS)
	gcc -o $@ $^ $(LOADLIBS)

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

.PHONY: test clean

clean:
	$(RM) *.o  *.exe
