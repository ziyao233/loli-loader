// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/eficall.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_EFICALL_H_INC__
#define __LOLI_EFICALL_H_INC__

#if defined(LOLI_TARGET_RISCV64) || defined(LOLI_TARGET_LOONGARCH64) || \
    defined(LOLI_TARGET_AARCH64)
#define efi_call(f, ...) ((f)(__VA_ARGS__))
#elif defined(LOLI_TARGET_X86_64)

int __efi_call1(unsigned long int, ...);
int __efi_call2(unsigned long int, ...);
int __efi_call3(unsigned long int, ...);
int __efi_call4(unsigned long int, ...);
int __efi_call5(unsigned long int, ...);
int __efi_call6(unsigned long int, ...);
int __efi_call10(unsigned long int, ...);

#define __arg(x) ((unsigned long int)(x))
#define _efi_call1(x, a) \
	__efi_call1(__arg(a), __arg(x))
#define _efi_call2(x, a, b) \
	__efi_call2(__arg(a), __arg(b), __arg(x))
#define _efi_call3(x, a, b, c) \
	__efi_call3(__arg(a), __arg(b), __arg(c), __arg(x))
#define _efi_call4(x, a, b, c, d) \
	__efi_call4(__arg(a), __arg(b), __arg(c), __arg(d), __arg(x))
#define _efi_call5(x, a, b, c, d, e) \
	__efi_call5(__arg(a), __arg(b), __arg(c), __arg(d), __arg(e), __arg(x))
#define _efi_call6(x, a, b, c, d, e, f) \
	__efi_call6(__arg(a), __arg(b), __arg(c), __arg(d), __arg(e), __arg(f), __arg(x))
#define _efi_call10(x, a, b, c, d, e, f, g, h, i, j) \
	__efi_call10(__arg(a), __arg(b), __arg(c), __arg(d), __arg(e),	\
		     __arg(f), __arg(g), __arg(h), __arg(i), __arg(j), __arg(x))

#define __efi_call_nargs(a, b, c, d, e, f, g, h, i, j, n, ...) n
#define efi_call_nargs(...) \
	__efi_call_nargs(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,)

#define __efi_call_concat(a, b) a##b
#define efi_call_concat(a, b) __efi_call_concat(a, b)

#define efi_call(f, ...) \
	efi_call_concat(_efi_call,efi_call_nargs(__VA_ARGS__))(f, __VA_ARGS__)

#else
#error "Unknown target"
#endif

#define efi_method(p, f, ...) efi_call((p)->f, (p), __VA_ARGS__)

#endif	// __LOLI_EFICALL_H_INC__
