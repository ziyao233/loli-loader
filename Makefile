#	SPDX-License-Identifier: MPL-2.0
#	loli-loader
#	/Makefile
#	Copyright (c) 2024-2025 Yao Zi.

ARCH		= $(shell uname -m)
CC		= cc
CCAS		= cc
CCLD		= cc
PYTHON		= python

CONFIG_$(ARCH)	= yes

ARCHFLAGS_$(CONFIG_x86_64)	= -DLOLI_TARGET_X86_64 -mgeneral-regs-only

# For AArch64, UEFI Specification 2.10 states
#	Floating point and SIMD instructions may be used.
# But it's not sure whether it's widely followed among firmware. Let's try not
# to be the trouble maker.
ARCHFLAGS_$(CONFIG_aarch64)	= -DLOLI_TARGET_AARCH64 -mgeneral-regs-only

# Notice for riscv64: UEFI requires extensions are checked before usage, so
# it's important not to pass a -march argument with baseline higher than your
# distribution's requirement.
ARCHFLAGS_$(CONFIG_riscv64)	= -DLOLI_TARGET_RISCV64

# For LoongArch, UEFI Specification (again 2.10) states
#	FP unit can be used(CSR.EUEN.FPE to enable), calling convention refer
#	to 2.3.8.2.
# But it's unclear whether SIMD instructions (LSX, LASX) are allowed.
ARCHFLAGS_$(CONFIG_loongarch64)	= -DLOLI_TARGET_LOONGARCH64 -mno-lsx -mno-lasx

ifeq ($(DEBUG),)
DEBUG_FLAGS	:= -O2
else
DEBUG_FLAGS	:= -O0 -g
endif

MYCFLAGS	?= -ffreestanding -fno-stack-protector -fno-stack-check \
		   -fPIE -fshort-wchar -static -nostdinc -std=c99	\
		   -Wall						\
		   $(DEBUG_FLAGS) $(ARCHFLAGS_yes) $(CFLAGS)

MYCCASFLAGS	?= $(MYCFLAGS) $(CCASFLAGS)
MYLDFLAGS	= $(LDFLAGS)

OBJS		= src/loli.o src/efi.o src/string.o src/interaction.o
OBJS		+= src/memory.o src/file.o src/misc.o src/extlinux.o
OBJS		+= src/eficall.o src/entry.o src/graphics.o src/serial.o
OBJS		+= src/font.o src/ctype.o

default: loli.efi

loli.efi: loli.elf
	$(PYTHON) tools/elf2efi.py loli.elf loli.efi

loli.elf: $(OBJS)
	$(CCLD) -fPIE -o $@ $(MYLDFLAGS) $(OBJS) -nodefaultlibs -nostartfiles

%.o: %.c
	$(CC) $(MYCFLAGS) -c $< -o $@ -Iinclude

%.o: %.S
	$(CCAS) $(MYCCASFLAGS) -c $< -o $@

clean:
	-rm $(OBJS)
