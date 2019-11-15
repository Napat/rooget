TOP_DIR = $(shell pwd)
export TOP_DIR

# CROSS_COMPILE=/opt/codefidence/bin/mipsel-linux-
AR              = $(CROSS_COMPILE)ar
AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CXX             = $(CROSS_COMPILE)g++
CPP             = $(CROSS_COMPILE)cpp
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
SSTRIP          = $(CROSS_COMPILE)sstrip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump
RANLIB          = $(CROSS_COMPILE)ranlib

CFLAGS=-c -Wall
#COMMON_LIB = -lpthread -lm -DON_LINUX

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
	
all: rooget

rooget: $(OBJS) 
	echo $(OBJS) 
	@$(CC) $(OBJS) -o rooget

# main.o: main.c
# 	@$(CC) $(CFLAGS) main.c

%.o: %.c
	$(CC) $(CFLAGS) $(COMMON_LIB) -shared -o $@ $<

# $(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
# 	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf *.o rooget *.txt
