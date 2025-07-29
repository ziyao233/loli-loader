#	SPDX-License-Identifier: MPL-2.0
#	loli-loader
#	/Makefile
#	Copyright (c) 2024 Yao Zi.

ARCH		= $(shell uname -m)
CC		= clang
CCAS		= clang
CCLD		= clang
OBJCOPY		= objcopy
PYTHON		= python

CONFIG_$(ARCH)	= yes
ARCHFLAGS_$(CONFIG_x86_64)	= -DLOLI_TARGET_X86_64
ARCHFLAGS_$(CONFIG_riscv64)	= -DLOLI_TARGET_RISCV64

MYCFLAGS	?= -ffreestanding -fno-stack-protector -fno-stack-check \
		   -fPIE -fshort-wchar -static -nostdinc		\
		   $(ARCHFLAGS_yes) $(CFLAGS)

MYCCASFLAGS	?= $(MYCFLAGS) $(CCASFLAGS)
MYLDFLAGS	= $(LDFLAGS)

OBJS		= src/loli.o src/efi.o src/string.o src/interaction.o
OBJS		+= src/memory.o src/file.o src/misc.o src/extlinux.o
OBJS		+= src/eficall.o src/entry.o

default: loli.efi

loli.efi: loli.elf
	$(PYTHON) tools/elf2efi.py loli.elf loli.efi

loli.elf: $(OBJS)
	$(CCLD) -fPIE -o $@ $(MYLDFLAGS) $(OBJS) -nodefaultlibs -nostartfiles

%.o: %.c
	$(CC) $(MYCFLAGS) -c $< -o $@ -Iinclude

%.o: %.S
	$(CCAS) $(MYCCASFLAGS) -c $< -o $@

src/begin.o: src/begin.S
	$(CCAS) -fPIC -fpie -c $< -o $@

clean:
	-rm $(OBJS)
