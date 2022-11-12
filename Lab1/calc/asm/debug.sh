#/bin/bash
# .asm -> .o -> execuable file

set -e

nasm -f elf32 -g "$1.asm";
mv "$1.o" "../obj/$1.o";
gcc -g -o "debug/$1" "../obj/$1.o";
gdb "debug/$1";
