# A Makefile to generate executive file                                                                                                                                                     
# Copyright (C) 2017 sinbest international technology

CC = gcc

ROOT := $(shell pwd)
INCLUDE := $(ROOT)/common
SRC := $(ROOT)

SUB_DIR := cgi server

all:$(SUB_DIR)
#$(SUB_DIR):ECHO
	@echo +++Start Compiling...
	@for n in $(SUB_DIR);do $(MAKE) -C $$n;done
#	make -C $@

#ECHO:
#	@echo $(SUB_DIR)

clean:
	@echo +++Start Cleaning...
	@for n in $(SUB_DIR);do $(MAKE) -C $$n clean;done
	-rm -rf $(INCLUDE)/*.bak
	-rm -rf $(ROOT)/*.bak

.PHONY: all clean