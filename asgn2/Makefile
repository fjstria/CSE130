# FJ Tria (@fjstria)
# CSE130/asgn2/Makefile

CC = clang
CFLAGS = -Wall -Wextra -Werror -pedantic
OBJS = httpserver.o request.o asgn2_helper_funcs.a

all: httpserver

httpserver: $(OBJS)
	$(CC) -o httpserver $(OBJS)

httpserver.o: httpserver.c
	$(CC) $(CFLAGS) -c httpserver.c

request.o: 
	$(CC) $(CFLAGS) -c request.c

clean:
	rm -f httpserver httpserver.o global.o request.o