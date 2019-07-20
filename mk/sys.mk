CC	= gcc
STRIP	= strip
CP	= cp
RM	= rm -f
CDEFS	=
LDFLAGS	=
CFLAGS	= $(DBG) $(CDEFS) $(TARGET_CFLAGS)

ifeq ($(shell uname -s | grep -qi "mingw\|msys_nt" && echo yes),yes)
TARGET_SYSNAME		:= Windows
TARGET_TOOLCHAINNAME	:= MinGW
else ifeq ($(shell uname),Linux)
TARGET_SYSNAME		:= POSIX
TARGET_TOOLCHAINNAME	:= GNU
endif

ifndef TARGET_SYSNAME
$(error TARGET_SYSNAME not defined.)
endif

ifndef TARGET_TOOLCHAINNAME
$(error TARGET_TOOLCHAINNAME not defined.)
endif

#--------------------

ifeq ($(TARGET_SYSNAME),Windows)
EXESFX=.exe
endif

ifeq ($(TARGET_TOOLCHAINNAME),MinGW)
OBJSFX=.o
TARGET_CFLAGS		:= -Wall --input-charset=utf-8 --exec-charset=cp932
endif

ifeq ($(TARGET_SYSNAME),POSIX)
EXESFX=
OBJSFX=.o
TARGET_CFLAGS		:= -Wall --input-charset=utf-8
endif

ifdef DEBUG
  DBG		:= -g
else
  DBG		:= -O2
  IS_STRIP	:= 1
endif

EXEDIR	= ../../bin
