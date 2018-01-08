SOURCES = $(wildcard *.c)
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

all: app

app: LOADLIBS = -lssl -lcrypto
app: $(OBJECTS)
	gcc -o $@ $^ $(LOADLIBS)

test: test_util test_list test_http_headers

test_util: CFLAGS = -DTEST_UTIL -Wall -g
test_util: util.o
	gcc -o $@ $^

test_list: CFLAGS = -DTEST_LIST -Wall -g
test_list: list.o
	gcc -o $@ $^

test_http_headers: CFLAGS = -DTEST_HTTP_HEADERS -Wall -g
test_http_headers: http_headers.o list.o
	gcc -o $@ $^

test_string_buffer: CFLAGS = -DTEST_STRING_BUFFER -Wall -g
test_string_buffer: string_buffer.o
	gcc -o $@ $^

.PHONY: test clean

clean:
	$(RM) *.o
