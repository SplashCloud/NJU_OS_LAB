#/bin/bash
# .asm -> .o -> execuable file

set -e

nasm -f elf32 "$1.asm" -o "../obj/$1.o";
gcc -o "../exe/$1" "../obj/$1.o";
./"../exe/$1";
