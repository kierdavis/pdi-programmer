#!/bin/sh

set -o errexit -o pipefail -o nounset

platform=$1
source ./common.sh

objfiles=""

cfiles=$(find $src_dirs -name '*.c')
for cfile in $cfiles; do
  objfile=$build_dir/$(basename $cfile | sed 's/.c/.o/')
  objfiles="$objfiles $objfile"
  show avr-gcc $CC_FLAGS $C_FLAGS -o $objfile -c $cfile
done

cppfiles=$(find $src_dirs -name '*.cpp')
for cppfile in $cppfiles; do
  objfile=$build_dir/$(basename $cppfile | sed 's/.cpp/.o/')
  objfiles="$objfiles $objfile"
  show avr-g++ $CC_FLAGS $CPP_FLAGS -o $objfile -c $cppfile
done

elf=$build_dir/pdiprog.elf
hex=$build_dir/pdiprog.hex
show avr-g++ $LD_FLAGS -o $elf $objfiles
show avr-objcopy -O ihex $elf $hex

show avr-size $elf
