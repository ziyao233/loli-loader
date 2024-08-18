#	SPDX-License-Identifier: MPL-2.0
#	loli-loader
#	/Makefile
#	Copyright (c) 2024 Yao Zi.

ARCH		?= $(shell uname -m)

CC		= clang -target riscv64-unknown-linux
CCAS		= clang -target riscv64-unknown-linux
LD		= ld.lld
OBJCOPY		= objcopy

LINK_SCRIPT	= loli.ld

CFLAGS		?= -ffreestanding -fno-stack-protector -fno-stack-check \
		   -fpie -fPIC -fshort-wchar -nostdinc
CCASFLAGS	?= $(CFLAGS)
LDFLAGS		= -T$(LINK_SCRIPT)

default: loli.efi

loli.efi: loli.so
	$(OBJCOPY) -O binary loli.so loli.efi \
		-j .text -j .rodata -j .data

loli.so: loli.o begin.o
	$(LD) -o $@ $(LDFLAGS) loli.o begin.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CCAS) $(CCASFLAGS) -c $< -o $@