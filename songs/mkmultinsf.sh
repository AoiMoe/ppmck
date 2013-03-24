#!/bin/sh

#
# Usage : ./mkmultinsf.sh songA.mml songB.mml ....
#

EXEDIR=`dirname $0`

export NES_INCLUDE="$EXEDIR/../nes_include"

$EXEDIR/../bin/ppmckc -m1 -i -u $*

if [ ! -s effect.h ];
then
	exit
fi

$EXEDIR/../bin/nesasm -raw ppmck.asm

if [ ! -s ppmck.nes ];
then
	exit
fi

mv ppmck.nes multisong.nsf

if [ $# -gt 1 ];
then
	MODE=$2
fi

rm define.inc
rm effect.h


