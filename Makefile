TARGETS=main

CFLAGS=-O0 -g -Wall -Wvla -Werror -Wno-error=unused-variable

all: $(TARGETS)

main: main.c server.c
	gcc $(CFLAGS) -o main main.c server.c -lpthread

clean:
	rm -f $(TARGETS)
