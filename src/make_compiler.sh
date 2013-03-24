#!/bin/sh
#
# usage : make_compiler.sh [install]
#

cd nesasm ; make -f Makefile.unx $*
cd ../ppmckc ; make -f Makefile.UNIX $*

