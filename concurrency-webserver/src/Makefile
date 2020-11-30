# An admittedly primitive Makefile
# To compile, type "make" or make "all"
# To remove files, type "make clean"

CC = gcc
CFLAGS = -Wall -I. -pthread
OBJS = wserver.o wclient.o request.o io_helper.o thread_wserver.o

.SUFFIXES: .c .o 

all: wserver wclient spin.cgi thread_wserver

wserver: wserver.o request.o io_helper.o
	$(CC) $(CFLAGS) -o wserver wserver.o request.o io_helper.o 

wclient: wclient.o io_helper.o
	$(CC) $(CFLAGS) -o wclient wclient.o io_helper.o

spin.cgi: spin.c
	$(CC) $(CFLAGS) -o spin.cgi spin.c

thread_wserver: thread_wserver.o request.o io_helper.o
	$(CC) $(CFLAGS) -o thread_wserver thread_wserver.o request.o io_helper.o 

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	-rm -f $(OBJS) wserver wclient spin.cgi
