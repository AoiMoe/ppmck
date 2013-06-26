#!/bin/sh

sh make_compiler.sh clean install 
sh make_compiler.sh clean 
sh make_compiler_w32.sh clean install
sh make_compiler_w32.sh clean 

