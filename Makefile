
TARGET = alru
FILES += ./lru.c ./main.c
OBJS += ./*.o

SHELL= /bin/sh
CC_OSX = clang
CC_LINUX = gcc

UNAME=$(shell uname)

TFLAGS=-D_T_LRU
DFLAGS=-g -Wall -o0

CC :=
ifeq ($(UNAME), Darwin)
	CC := $(CC_OSX)
else
	CC := $(CC_LINUX)
endif


$(TARGET):
	$(CC) $(DFLAGS) $(TFLAGS) -o $(TARGET) $(FILES)


.PHONY clean:
	rm -rf $(TARGET) $(OBJS) $(TARGET).dSYM

