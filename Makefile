#
# Makefile for the UM Test lab
# 
CC = gcc

CFLAGS  = -O2 -flto -g -std=gnu99 -Wall -Wextra -Werror -pedantic
LDFLAGS = -g
LDLIBS  = -O2 -lm

EXECS   = proxy clean

all: $(EXECS)

proxy: proxy.o HttpReqParser.o HttpResParser.o HttpCache.o CacheObject.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# To get *any* .o file, compile its .c file with the following rule.

INCLUDES = $(shell echo *.h)

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXECS)  *.o

