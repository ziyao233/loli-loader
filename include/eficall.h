// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/eficall.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_EFICALL_H_INC__
#define __LOLI_EFICALL_H_INC__

#ifdef LOLI_TARGET_RISCV64
#define efi_call(f, ...) ((f)(__VA_ARGS__))
#else
#error "Unknown target"
#endif

#define efi_method(p, f, ...) efi_call((p)->f, (p), __VA_ARGS__)

#endif	// __LOLI_EFICALL_H_INC__
