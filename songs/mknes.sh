#!/bin/sh
#
# build NES file from MML

#
# Usage : ./mknes.sh songfile.mml [DEBUG]
#

SONGBASE=`basename $1 .mml`
SONGDIR=`dirname $1`
SONGOUT=$SONGBASE

EXEDIR=`dirname $0`

# check DEBUG flag
#
if [ $# -gt 1 ]
then
    MODE=$2
fi

#
# set include directory
#
export NES_INCLUDE="$EXEDIR/../nes_include"

#
# move to song directory
#
cd $SONGDIR

#
# compile
#
$EXEDIR/../bin/ppmckc -m1 -i $SONGBASE.mml

if [ ! -s effect.h ]
then
    echo "effect.h is not found."
    exit 1
fi

#
# add .list
#
if [ "x$MODE" == "xDEBUG" ]
then
    echo "    .list" >> define.inc
fi

$EXEDIR/../bin/nesasm -raw ppmck.asm

if [ ! -s ppmck.nes ]
then
    echo "ppmck.nes is not found."
    exit 1
fi

mv ppmck.nes $SONGOUT.nsf

if [ "x$MODE" != "xDEBUG" ]
then
    rm $SONGBASE.h
    rm define.inc
    rm effect.h
fi

exit 0

