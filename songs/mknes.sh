#!/bin/sh

#
# Usage : ./mknes.sh songfile.mml [DEBUG]
#

FBASE=`basename $1 .mml`
FDIR=`dirname $1`
FIN=$FDIR/$FBASE
FOUT=$FBASE

EXEDIR=`dirname $0`

export NES_INCLUDE="$EXEDIR/../nes_include"

$EXEDIR/../bin/ppmckc -m1 -i $FIN.mml

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

mv ppmck.nes $FOUT.nes

if [ $# -gt 1 ];
then
	MODE=$2
fi

if [ "x$MODE" != "xDEBUG" ];
then
	rm $FIN.h
	rm define.inc
	rm effect.h
fi

