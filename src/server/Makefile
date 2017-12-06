# A Makefile to generate executive file                                                                                                                                                     
# Copyright (C) 2017 sinbest international technology

CC = gcc  

ROOT := $(shell pwd)/..
INCLUDE := $(ROOT)/common
CFLAGS += -I$(INCLUDE)

all:anyconn
.PHONY:all  

anyconn:backend.c main.c
	$(CC) -o anyconn backend.c main.c $(CFLAGS)

clean:
	-rm -rf *.o anyconn *.bak