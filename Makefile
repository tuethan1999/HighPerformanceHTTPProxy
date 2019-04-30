#
# Makefile for the UM Test lab
# 
CC = gcc

CFLAGS  = -O2 -flto -g -std=gnu99 -Wall -Wextra -pedantic
LDFLAGS = -g
LDLIBS  = -O2 -lm

EXECS   = proxy cache clean

all: $(EXECS)

proxy: proxy.o HttpReqParser.o HttpResParser.o HttpCache.o CacheObject.o Buffer.o Bucket.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)
cache: cache_tests.o HttpCache.o CacheObject.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)
# To get *any* .o file, compile its .c file with the following rule.

INCLUDES = $(shell echo *.h)

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXECS)  *.o

