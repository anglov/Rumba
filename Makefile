CC=gcc
CFLAGS=-Wall -std=c99 -pthread
LFLAGS=
OBJS=udp.o tcp.o list.o threadsoper.o tcpclient.o fileoper.o
all: main
main: $(OBJS)
	$(CC) $(LFLAGS) $(CFLAGS) main.c $^ -o dancer
clean:
	rm -rf *.o dancer
.PHONY: all clean

