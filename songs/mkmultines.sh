#!/bin/sh

#
# Usage : ./mkmultines.sh songA.mml songB.mml ....
#

EXEDIR=`dirname $0`

export NES_INCLUDE="$EXEDIR/../nes_include"

$EXEDIR/../bin/ppmckc -m1 -i -u $*

if [ ! -s effect.h ];
then
	exit
fi

echo "MAKE_NES .equ 1" >> define.inc

$EXEDIR/../bin/nesasm -raw ppmck.asm

if [ ! -s ppmck.nes ];
then
	exit
fi

mv ppmck.nes multisong.nes

if [ $# -gt 1 ];
then
	MODE=$2
fi

rm define.inc
rm effect.h


