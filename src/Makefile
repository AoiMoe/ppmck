#
# Makefile for bintools/compiler
#
# Usage : make [cross] [cross_full] [def_full]
#
# cross : make a compiler for win32 and host 
# cross_full : make a compiler and tools for win32 and host
#
# def_full : make a compiler and tools for host(default environtment)
#


#
# defines
#

W32PFX = i586-mingw32-

MAKE_W32B = make -f Makefile.unx CC=$(W32PFX)gcc EXESFX=.exe

MAKE_W32C = make -f Makefile.UNIX \
 CC=$(W32PFX)gcc STRIPER=$(W32PFX)strip \
 CDEFS="--input-charset=utf-8 --exec-charset=cp932" EXESFX=.exe

MAKE_DEFB = make -f Makefile.unx

MAKE_DEFC = make -f Makefile.UNIX


#
# basic sets
#

all : def_compiler 

clean : def_clean_c



cross : clean_w32_start w32_compiler w32_clean def_compiler def_clean

cross_full : clean_w32_start w32_tools w32_clean  def_tools def_clean

def_full : clean_def_start def_tools def_clean

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



