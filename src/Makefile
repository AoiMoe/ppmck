#
# Makefile for bintools/compiler
#
# Usage : make <current|compilers|fullset|current_full|clean_full>
#
#

OSX = 1
DEBUG = 0

ifeq ($(DEBUG),1)
CC = gcc -g
else
CC = gcc
endif


ifeq ($(OSX),1)

# SYSROOT = --sysroot=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.6.sdk/
OPTS = -mmacosx-version-min=10.6 -m32
DEF_CC = CC="$(CC) $(OPTS) $(SYSROOT)"

endif



#
# defines
#

W32PFX = i586-mingw32-

MAKE_W32B = make -f Makefile.unx CC=$(W32PFX)$(CC) EXESFX=.exe

MAKE_W32C = make -f Makefile.UNIX \
 CC=$(W32PFX)$(CC) STRIPER=$(W32PFX)strip \
 CDEFS="--input-charset=utf-8 --exec-charset=cp932" EXESFX=.exe

MAKE_DEFB = make $(DEF_CC) -f Makefile.unx

MAKE_DEFC = make $(DEF_CC) -f Makefile.UNIX


#
# basic sets
#

all : 
	@echo "Usage : make <current|compilers|fullset|current_full|clean_full>"
	@echo " current : ppmckc for current enviroment"
	@echo " compilers : ppmckc for cross and current enviroment"
	@echo " fullset : ppmckc and nesasm for cross and current enviroment"
	@echo " current_full : ppmckc and nesasm for current enviroment"
	@echo " OSX=0 : no use OS X flags for compiling"
	@echo " DEBUG=1 : make debuggable binary"


current : def_compiler

compilers : clean_w32_start w32_compiler w32_clean def_compiler def_clean

fullset : clean_w32_start w32_tools w32_clean  def_tools def_clean

current_full : clean_def_start def_tools def_clean

clean : def_clean

clean_full : def_clean w32_clean





#
# compiling
#

.PHONY: w32_bintools def_bintools w32_compiler def_compiler


tools : def_tools

def_tools : def_bintools def_compiler

w32_tools : w32_bintools w32_compiler


bintools: w32_bintools def_bintools

compiler: w32_compiler def_compiler


w32_bintools: 
	cd nesasm ; $(MAKE_W32B) install
	
w32_compiler:
	cd ppmckc ; $(MAKE_W32C) install
 
def_bintools:
	cd nesasm ; $(MAKE_DEFB) install

def_compiler: 
	cd ppmckc ; $(MAKE_DEFC) install

#
# cleaning
#

.PHONY: def_clean_b def_clean_c w32_clean_b w32_clean_c

def_clean: def_clean_b def_clean_c

def_clean_b: 
	cd nesasm ; $(MAKE_DEFB) clean

def_clean_c: 
	cd ppmckc ; $(MAKE_DEFC) clean
	
w32_clean: w32_clean_b w32_clean_c

w32_clean_b: 
	cd nesasm ; $(MAKE_W32B) clean

w32_clean_c: 
	cd ppmckc ; $(MAKE_W32C) clean

clean_w32_start:
	cd nesasm ; $(MAKE_W32B) clean
	cd ppmckc ; $(MAKE_W32C) clean

clean_def_start:
	cd nesasm ; $(MAKE_DEFB) clean
	cd ppmckc ; $(MAKE_DEFC) clean


clean_full:
	cd nesasm ; $(MAKE_W32B) clean
	cd ppmckc ; $(MAKE_W32C) clean
	cd nesasm ; $(MAKE_DEFB) clean
	cd ppmckc ; $(MAKE_DEFC) clean



