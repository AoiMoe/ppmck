CC	= gcc
STRIP	= strip
CP	= cp
RM	= rm -f
CDEFS	=
LDFLAGS	=
CFLAGS	= $(DBG) $(CDEFS) $(TARGET_CFLAGS)

ifeq ($(shell uname -s | grep -qi mingw && echo yes),yes)
TARGET_SYSNAME		:= Windows
TARGET_TOOLCHAINNAME	:= MinGW
TARGET_CFLAGS		:= -Wall --input-charset=utf-8 --exec-charset=cp932
endif

ifeq ($(TARGET_SYSNAME),Windows)
EXESFX=.exe
endif

ifeq ($(TARGET_TOOLCHAINNAME),MinGW)
OBJSFX=.o
endif

ifdef DEBUG
  DBG		:= -g
else
  DBG		:= -O2
  IS_STRIP	:= 1
endif

EXEDIR	= ../../bin
