TARGETS=sever

CFLAGS=-O0 -g -Wall -Wvla -Werror -Wno-error=unused-variable

all: $(TARGETS)

sever: server.c
	gcc $(CFLAGS) -o server server.c -lpthread

clean:
	rm -f $(TARGETS)
