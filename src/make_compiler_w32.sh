#!/bin/sh
#
# usage : make_compiler_w32.sh [install]
#

PFX=i586-mingw32-

cd nesasm ; make -f Makefile.unx CC=${PFX}gcc EXESFX=.exe $*
cd ../ppmckc ; make -f Makefile.UNIX \
 CC=${PFX}gcc STRIPER=${PFX}strip \
 CDEFS="--input-charset=utf-8 --exec-charset=cp932" EXESFX=.exe $*

