#!/bin/bash
rm -rf *.c
build/bin/mlton -codegen c -keep g -target self -verbose 3 $@
gcc -pthread -g3 -m32 *.c -o eq  -Ibuild/lib/targets/self/include -Ibuild/lib/include -lgmp -Lbuild/lib/targets/self/ -lmlton -lgdtoa-pic -l gdtoa -lgdtoa-gdb -lmlton-pic -lm
