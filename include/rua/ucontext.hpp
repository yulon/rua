#ifndef _RUA_UCONTEXT_HPP
#define _RUA_UCONTEXT_HPP

#include "macros.hpp"

#if defined(RUA_AMD64) || defined(RUA_I386)

#include "any_word.hpp"
#include "bytes.hpp"
#include "code_fn.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstdint>
#include <type_traits>

namespace rua {

/*
	Reference from
		https://github.com/skywind3000/collection/tree/master/ucontext
		https://github.com/hnes/libaco/blob/master/acosw.S
		https://github.com/boostorg/ucontext/tree/develop/src/asm
*/

struct uc_regs_t {
#if defined(RUA_AMD64) || defined(RUA_I386)
	uintptr_t a;	 // 0
	uintptr_t b;	 // 4, 8
	uintptr_t c;	 // 8, 16
	uintptr_t d;	 // 12, 24
	uintptr_t si;	// 16, 32
	uintptr_t di;	// 20, 40
	uintptr_t sp;	// 24, 48
	uintptr_t bp;	// 28, 56
	uintptr_t ip;	// 32, 64
	uintptr_t flags; // 36, 72
#ifdef RUA_AMD64
	uintptr_t r8;  // 80
	uintptr_t r9;  // 88
	uintptr_t r10; // 96
	uintptr_t r11; // 104
	uintptr_t r12; // 112
	uintptr_t r13; // 120
	uintptr_t r14; // 128
	uintptr_t r15; // 136
#endif
	uint32_t mxcsr; // 40, 144
	uint16_t fcw;   // 44, 148
	uint16_t _padding;
#endif
};
#ifdef RUA_AMD64
RUA_SASSERT(sizeof(uc_regs_t) == 152);
#elif defined(RUA_I386)
RUA_SASSERT(sizeof(uc_regs_t) == 48);
#endif

struct uc_stack_t {
	uintptr_t base;
	uintptr_t limit;
};

struct ucontext_t {
	uc_regs_t regs;
	uc_stack_t stack;
};

RUA_CODE(_get_ucontext_code){
#ifdef RUA_AMD64
#ifdef RUA_MS64_FASTCALL
#include "ucontext/get_amd64_win.inc"
#else
#include "ucontext/get_amd64_sysv.inc"
#endif
#elif defined(RUA_I386)
#ifdef _WIN32
#include "ucontext/get_i386_win.inc"
#else
#include "ucontext/get_i386.inc"
#endif
#endif
};

RUA_CODE(_set_ucontext_code){
#ifdef RUA_AMD64
#ifdef RUA_MS64_FASTCALL
#include "ucontext/set_amd64_win.inc"
#else
#include "ucontext/set_amd64_sysv.inc"
#endif
#elif defined(RUA_I386)
#ifdef _WIN32
#include "ucontext/set_i386_win.inc"
#else
#include "ucontext/set_i386.inc"
#endif
#endif
};

RUA_CODE(_swap_ucontext_code){
#ifdef RUA_AMD64
#ifdef RUA_MS64_FASTCALL
#include "ucontext/swap_amd64_win.inc"
#else
#include "ucontext/swap_amd64_sysv.inc"
#endif
#elif defined(RUA_I386)
#ifdef _WIN32
#include "ucontext/swap_i386_win.inc"
#else
#include "ucontext/swap_i386.inc"
#endif
#endif
};

RUA_CODE_FN(bool(ucontext_t *ucp), get_ucontext, _get_ucontext_code)

RUA_CODE_FN(void(const ucontext_t *ucp), set_ucontext, _set_ucontext_code)

RUA_CODE_FN(
	void(ucontext_t *oucp, const ucontext_t *ucp),
	swap_ucontext,
	_swap_ucontext_code)

inline void make_ucontext(
	ucontext_t *ucp,
	void (*func)(any_word),
	any_word func_param,
	bytes_ref stack) {

	auto stk_end = stack.data() + stack.size();
	ucp->regs.sp = stk_end.uintptr() - 5 * sizeof(uintptr_t);
	ucp->regs.ip = reinterpret_cast<uintptr_t>(func);

#ifdef RUA_AMD64
#ifdef RUA_MS64_FASTCALL
	ucp->regs.c
#else
	ucp->regs.di
#endif
#elif defined(RUA_I386)
	reinterpret_cast<uintptr_t *>(ucp->regs.sp)[2]
#endif
		= func_param;

	ucp->stack.base = stk_end;
	ucp->stack.limit = stack.data();
}

} // namespace rua

#else

#include <ucontext.h>

namespace rua {

using ucontext_t = ::ucontext_t;

RUA_FORCE_INLINE bool get_ucontext(ucontext_t *ucp) {
	return !getcontext(ucp);
}

static void (*set_ucontext)(const ucontext_t *ucp) = &setcontext;

static void (*swap_ucontext)(ucontext_t *oucp, const ucontext_t *ucp) =
	&swapcontext;

inline void make_ucontext(
	ucontext_t *ucp,
	void (*func)(any_word),
	any_word func_param,
	bytes_ref stack) {
	ucp->uc_link = nullptr;
	ucp->uc_stack.ss_sp = stack.data();
	ucp->uc_stack.ss_size = stack.size();
	makecontext(ucp, func, 1, func_param);
}

} // namespace rua

#endif

#endif
