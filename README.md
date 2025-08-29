# The loli Bootloader

A bootloader meant to be small and useful for experienced users. Just like
systemd-boot, most functionaility is implemented through UEFI API.

***Currently WIP. Please wait for future releases, unless you're experienced
with UEFI and Linux bootflow.***

## Limitation

- Could read boot-files from ESP partition only. We could support misc
  parititions in the future, but are still limited by the types supported by
  UEFI firmware (most of the time this just means FAT-family).
- Loads EFI binaries only (for Linux kernels, EFIstub must be enabled).
- For devicetree systems, `EFI_DT_FIXUP_PROTOCOL_GUID` must be implemented in
  firwmare to correctly fix up devicetrees, since we don't carry a libfdt.
- initrd is passed through `LINUX_EFI_INITRD_MEDIA_GUID` configuration table,
  which is only supported by Linux 6.1 or later.
- Building involves a Python script taken from systemd-boot, there's a plan to
  rewrite it in Lua or C.
- Extlinux-style configuration. I don't want to create one more new format.

![xkcd 927: standards](https://imgs.xkcd.com/comics/standards.png)

## Supported Platforms

- x86_64 (amd64): ACPI
- riscv64: devicetree and ACPI
- loongarch64: devicetree (untested) and ACPI

## Configuration File

The loli bootloader searches for `/loli.cfg` in ESP as its configuration file.
Its format is extlinux-like, for example,

```
timeout 3

label Linux Boot Test
	kernel  /vmlinux-riscv
	initrd  /initramfs.gz
	append  earlycon rootwait console=ttyS0 loglevel=9 memblock=debug

label Linux Boot Test Quiet
	kernel  /vmlinux-riscv
	initrd  /initramfs.gz
	append  quiet rootwait console=ttyS0
```

### Supported keys outside a label

- `timeout`: Specify timeout before booting the first entry. `0` means no
  timeout and is the default value.

### Supported keys inside a label

- `kernel`
- `initrd`: Optional
- `append`: Optional

### Treatment to malformed entries

As long as the entry cannot be understood by loli, it's automatically skipped.

## How to Build

You need to install

- GCC or Clang
- Python 3
- Python package pyelftools

Simply run `make` in the top directory, which should soon complete with
`loli.efi` produced.

### Useful Targets

- `loli.efi`: The UEFI application in PE format. This is the default.
- `loli.elf`: The ELF application, should be converted to a PE binary (by
  using `tools/elf2efi.py`) before booting.
- `clean`: Clean the project up.

### Useful Makefile Variables

- `ARCH`: Specify the architecture to build. This variable could be
  automatically detected with `uname -m`. Possible values:
  - `x86_64`
  - `riscv64`
  - `loongarch64`
- `CC`: Target compiler.
- `CCAS`: Target assembler.
- `CCLD`: Target linker.
- `CFLAGS`: Add extra flags when building C files.
- `CCASFLAGS`: Add extra flags when assembling assembly.
- `LDFLAGS`: Add extra flags when linking the ELF file.
- `PYTHON`: Should point to a Python-3 compatible Python interpreter.

For cross-compilation, it's usually necessary to adjust `ARCH`, `CC`, `CCAS`
and `CCLD`. An exception is building with Clang and LLD, where you could
alternate `ARCH`, `CFLAGS` and `LDFLAGS` only, LLVM will take care rest of the
work.

### Note

This is a relatively small project, I don't even bother writing proper
dependency tracking rules. Please run a clean build after making breaking
changes in the headers.

## License

This project is distributed under Mozilla Public License Version 2.0
(SPDX-License-Identifier: `MPL-2.0`).

## Warnings about UEFI

Long-term exposure to this code may cause loss of sanity, nightmares about
Windows-style API, or any other number other debilitating side effect. This
code is known to cause cancer, birth defects, and reproductive harm.
