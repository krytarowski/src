#!/bin/sh

./build.sh -U -N0 -j8 -u -O /public/netbsd-llvm -V MKGCC=no -V MKLLVM=yes -V HAVE_LLVM=yes -V MAKECONF=/dev/null distribution
