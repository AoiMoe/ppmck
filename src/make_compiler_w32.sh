#!/bin/sh
#
# usage : make_compiler_w32.sh [install]
#

cd nesasm ; make -f Makefile.unx CC=i386-mingw32-gcc EXESFX=.exe $*
cd ../ppmckc ; make -f Makefile.UNIX \
 CC=i386-mingw32-gcc STRIPER=i386-mingw32-strip \
 CDEFS="--input-charset=utf-8 --exec-charset=cp932" EXESFX=.exe $*

