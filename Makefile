#	SPDX-License-Identifier: MPL-2.0
#	loli-loader
#	/Makefile
#	Copyright (c) 2024 Yao Zi.

CC		= clang -target riscv64-unknown-linux
CCAS		= clang -target riscv64-unknown-linux
CCLD		= clang -target riscv64-unknown-linux
OBJCOPY		= objcopy
PYTHON		= python

CFLAGS		?= -ffreestanding -fno-stack-protector -fno-stack-check \
		   -fPIE -fshort-wchar -DLOLI_TARGET_RISCV64 -static	\
		   -nostdinc

CCASFLAGS	?= $(CFLAGS)
LDFLAGS		=

OBJS		= src/loli.o src/efi.o src/string.o src/interaction.o
OBJS		+= src/memory.o src/file.o src/misc.o src/extlinux.o

default: loli.efi

loli.efi: loli.elf
	$(PYTHON) tools/elf2efi.py loli.elf loli.efi

loli.elf: $(OBJS)
	$(CCLD) -fPIE -o $@ $(LDFLAGS) $(OBJS) -nodefaultlibs -nostartfiles

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -Iinclude

%.o: %.S
	$(CCAS) $(CCASFLAGS) -c $< -o $@

src/begin.o: src/begin.S
	$(CCAS) -fPIC -fpie -c $< -o $@

clean:
	-rm $(OBJS)
