#!/bin/zsh

riscv64-elf-gcc -nostdlib -T user.ld -o shell.elf shell.c
riscv64-elf-objcopy -O binary -j .text shell.elf shell.bin
riscv64-elf-ld -r -b binary -o shell.o shell.bin

#riscv64-elf-nm shell.elf | grep ' T '
#-T user.ld
#-Ttext=0x80200000 -e _start
