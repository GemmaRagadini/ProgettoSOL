#Makefile
CC = gcc
CFLAGS = -Wall -g -std=c99 -pthread
CWD= $(pwd)
CLIENTDIR = clientdir
SERVERDIR = serverdir

BINDIR = bin#contiene gli eseguibili

SRCS = $(wildcard $(SERVERDIR)/*.c)
SRCC = $(wildcard $(CLIENTDIR)/*.c)

OBJS := $(notdir $(SRCS:%.c=%.o))
OBJC := $(notdir $(SRCC:%.c=%.o))
OBJDIR = obj

.PHONY: all clean test1

all: server client

server: $(SERVERDIR)/$(OBJS)
	$(CC) $(CFLAGS) $< -o $@ 
	@mv server $(SERVERDIR)/server

client : $(CLIENTDIR)/$(OBJC)
	$(CC) $(CFLAGS) $< -o $@
	@mv client $(CLIENTDIR)/client


%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@


test1: server client
	sh test1.sh

test2: server client
	sh test2.sh

clean: 
	rm $(SERVERDIR)/$(OBJS) $(CLIENTDIR)/$(OBJC)

