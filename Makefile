#	SPDX-License-Identifier: MPL-2.0
#	loli-loader
#	/Makefile
#	Copyright (c) 2024 Yao Zi.

CC		= clang -target riscv64-unknown-linux
CCAS		= clang -target riscv64-unknown-linux
LD		= ld.lld
OBJCOPY		= objcopy

LINK_SCRIPT	= loli.ld

CFLAGS		?= -ffreestanding -fno-stack-protector -fno-stack-check \
		   -fpie -fPIC -fshort-wchar -nostdinc -DLOLI_TARGET_RISCV64
CCASFLAGS	?= $(CFLAGS)
LDFLAGS		= -T$(LINK_SCRIPT)

OBJS		= src/loli.o src/begin.o

default: loli.efi

loli.efi: loli.so
	$(OBJCOPY) -O binary loli.so loli.efi \
		-j .text -j .rodata -j .data

loli.so: $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -Iinclude

%.o: %.S
	$(CCAS) $(CCASFLAGS) -c $< -o $@
