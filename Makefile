#CROSS_COMPILE = 
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm

STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump

export AS LD CC CPP AR NM
export STRIP OBJCOPY OBJDUMP

CFLAGS := -Wall -Werror -O2 -g
CFLAGS += -I $(shell pwd)/Include

LDFLAGS := 

export CFLAGS LDFLAGS

TOPDIR := $(shell pwd)
export TOPDIR

TARGET := Divine

obj-y += main.o
obj-y += Common/
obj-y += Config/
#obj-y += Example/
#obj-y += Platform/
#obj-y += Business/
#obj-y += Components/
obj-y += Middleware/
obj-y += Thirdparty/

OBJ_DIR = Out

all : 
	rm -rf ${OBJ_DIR}
	mkdir ${OBJ_DIR}
	make -C ./ -f $(TOPDIR)/Makefile.build
	$(CC) $(LDFLAGS) -o $(TARGET) built-in.o -lpthread -lm -lrt
	rm -rf `find . -name built-in.o`
	mv -f `find . -name "*.o" -o -name "*.d"` ${OBJ_DIR}

clean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.d")
	rm -f $(TARGET)
	rm -rf Out
.PHONY:all clean
