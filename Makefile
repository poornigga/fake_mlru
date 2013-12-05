
TARGET = alru
FILES += ./*.c
OBJS += ./*.o

SHELL= /bin/sh
CC_OSX = clang
CC_LINUX = gcc

UNAME=$(shell uname)
CC :=
ifeq ($(UNAME), Darwin)
	CC := $(CC_OSX)
else
	CC := $(CC_LINUX)
endif

TFLAGS=-D_T_LRU
DFLAGS=-g -Wall -o0
LINKS=-lpthread

$(TARGET):
	$(CC) $(DFLAGS) $(TFLAGS) $(LINKS) -o $(TARGET) $(FILES)


.PHONY clean:
	rm -rf $(TARGET) $(OBJS) $(TARGET).dSYM 

