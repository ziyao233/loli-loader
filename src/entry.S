/*
 *	loli-loader
 *	/src/entry.S
 *	Copyright (c) 2025 Yao Zi.
 */

	.global		_start

_start:
#if defined(LOLI_TARGET_RISCV64)
	j		main
#elif defined(LOLI_TARGET_X86_64)
	movq		%rcx,		%rdi
	movq		%rdx,		%rsi
	jmp		main
#endif
