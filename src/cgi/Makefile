# A Makefile to generate executive file                                                                                                                                                     
# Copyright (C) 2017 sinbest international technology

CC = gcc

ROOT := $(shell pwd)/..
INCLUDE := $(ROOT)/common

CFLAGS += -I$(INCLUDE)
CFLAGS += -lfcgi

all:rpc.fcgi
.PHONY:all

rpc.fcgi:frontend.c rpc.c main.c
	$(CC) -o rpc.fcgi frontend.c rpc.c main.c $(CFLAGS)

clean:
	-rm -rf *.o rpc.fcgi *.bak