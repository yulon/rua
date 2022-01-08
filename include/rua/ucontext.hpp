/*
	Reference from
		https://github.com/skywind3000/collection/tree/master/vintage/context
		https://github.com/hnes/libaco/blob/master/acosw.S
		https://github.com/boostorg/context/tree/develop/src/asm
*/

#ifndef _RUA_UCONTEXT_HPP
#define _RUA_UCONTEXT_HPP

#include "bytes.hpp"
#include "generic_ptr.hpp"
#include "generic_word.hpp"
#include "macros.hpp"
#include "types/util.hpp"

#ifndef RUA_USING_NATIVE_UCONTEXT

#include "code_seg.hpp"

#ifdef RUA_X86

#ifdef _WIN32
#include <windows.h>
#endif

namespace rua {

struct ucontext_t {
	struct regs_t {
		uintptr_t a;	 // 0
		uintptr_t b;	 // 4, 8
		uintptr_t c;	 // 8, 16
		uintptr_t d;	 // 12, 24
		uintptr_t si;	 // 16, 32
		uintptr_t di;	 // 20, 40
		uintptr_t sp;	 // 24, 48
		uintptr_t bp;	 // 28, 56
		uintptr_t ip;	 // 32, 64
		uintptr_t flags; // 36, 72
#if RUA_X86 == 64
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
		uint16_t fcw;	// 44, 148
		uint16_t _padding;
	} regs;

	struct stack_t {
		generic_ptr base;
		generic_ptr limit;
	} stack;

	uintptr_t &sp() {
		return regs.sp;
	}

	const uintptr_t &sp() const {
		return regs.sp;
	}
};
#if RUA_X86 == 64
RUA_SASSERT(sizeof(uintptr_t) == 8);
RUA_SASSERT(sizeof(ucontext_t::regs_t) == 152);
#elif RUA_X86 == 32
RUA_SASSERT(sizeof(uintptr_t) == 4);
RUA_SASSERT(sizeof(ucontext_t::regs_t) == 48);
#endif

RUA_CODE(_get_ucontext_code) {
#if RUA_X86 == 64
#ifdef RUA_MS64_FASTCALL
#include "ucontext/get_amd64_win.inc"
#else
#include "ucontext/get_amd64_sysv.inc"
#endif
#elif RUA_X86 == 32
#ifdef _WIN32
#include "ucontext/get_i386_win.inc"
#else
#include "ucontext/get_i386.inc"
#endif
#endif
};

RUA_CODE(_set_ucontext_code) {
#if RUA_X86 == 64
#ifdef RUA_MS64_FASTCALL
#include "ucontext/set_amd64_win.inc"
#else
#include "ucontext/set_amd64_sysv.inc"
#endif
#elif RUA_X86 == 32
#ifdef _WIN32
#include "ucontext/set_i386_win.inc"
#else
#include "ucontext/set_i386.inc"
#endif
#endif
};

RUA_CODE(_swap_ucontext_code) {
#if RUA_X86 == 64
#ifdef RUA_MS64_FASTCALL
#include "ucontext/swap_amd64_win.inc"
#else
#include "ucontext/swap_amd64_sysv.inc"
#endif
#elif RUA_X86 == 32
#ifdef _WIN32
#include "ucontext/swap_i386_win.inc"
#else
#include "ucontext/swap_i386.inc"
#endif
#endif
};

RUA_CODE_FN(bool, get_ucontext, (ucontext_t * ucp), _get_ucontext_code)

RUA_CODE_FN(void, set_ucontext, (const ucontext_t *ucp), _set_ucontext_code)

RUA_CODE_FN(
	void,
	swap_ucontext,
	(ucontext_t * oucp, const ucontext_t *ucp),
	_swap_ucontext_code)

inline void make_ucontext(
	ucontext_t *ucp,
	void (*func)(generic_word),
	generic_word func_param,
	bytes_ref stack) {

	ucp->stack.base = stack.data() + stack.size();
	ucp->stack.base -= ucp->stack.base % sizeof(uintptr_t);
	ucp->stack.limit = stack.data();

	ucp->regs.sp = ucp->stack.base.uintptr() - 5 * sizeof(uintptr_t);
	ucp->regs.ip = reinterpret_cast<uintptr_t>(func);

#if RUA_X86 == 64
#ifdef RUA_MS64_FASTCALL
	ucp->regs.c
#else
	ucp->regs.di
#endif
#elif RUA_X86 == 32
	reinterpret_cast<uintptr_t *>(ucp->regs.sp)[2]
#endif
		= func_param;
}

} // namespace rua

#elif defined(RUA_ARM) && RUA_ARM == 32

namespace rua {

struct ucontext_t {
	uintptr_t r[16];

	uintptr_t &fp() {
		return r[11];
	}

	const uintptr_t &fp() const {
		return r[11];
	}

	uintptr_t &ip() {
		return r[12];
	}

	const uintptr_t &ip() const {
		return r[12];
	}

	uintptr_t &sp() {
		return r[13];
	}

	const uintptr_t &sp() const {
		return r[13];
	}

	uintptr_t &lr() {
		return r[14];
	}

	const uintptr_t &lr() const {
		return r[14];
	}

	uintptr_t &pc() {
		return r[15];
	}

	const uintptr_t &pc() const {
		return r[15];
	}
};

#include "switch_code_seg.h"

RUA_CODE(_get_ucontext_code){
#include "ucontext/get_arm.inc"
};

RUA_CODE(_set_ucontext_code){
#include "ucontext/set_arm.inc"
};

RUA_CODE(_swap_ucontext_code){
#include "ucontext/swap_arm.inc"
};

#include "switch_code_seg.h"

RUA_CODE_FN(bool, get_ucontext, (ucontext_t * ucp), _get_ucontext_code)

RUA_CODE_FN(void, set_ucontext, (const ucontext_t *ucp), _set_ucontext_code)

RUA_CODE_FN(
	void,
	swap_ucontext,
	(ucontext_t * oucp, const ucontext_t *ucp),
	_swap_ucontext_code)

inline void make_ucontext(
	ucontext_t *ucp,
	void (*func)(generic_word),
	generic_word func_param,
	bytes_ref stack) {
	auto stack_bottom = stack.data().uintptr() + stack.size();
	stack_bottom -= stack_bottom % sizeof(uintptr_t) + sizeof(uintptr_t);
	ucp->sp() = stack_bottom;
	ucp->lr() = reinterpret_cast<uintptr_t>(func);
	ucp->r[0] = func_param;
}

} // namespace rua

#else // ifdef {ARCH_MACRO}

#define RUA_USING_NATIVE_UCONTEXT

#endif // ifdef {ARCH_MACRO}

#endif // ifndef RUA_USING_NATIVE_UCONTEXT

#ifdef RUA_USING_NATIVE_UCONTEXT

#include <ucontext.h>

namespace rua {

struct ucontext_t : ::ucontext_t {
	generic_ptr _sp;

	generic_ptr &sp() {
		return _sp;
	}

	const generic_ptr &sp() const {
		return _sp;
	}
};

static RUA_NO_INLINE bool get_ucontext(ucontext_t *ucp) {
	volatile char v;
	ucp->_sp = &v;
	return ::getcontext(ucp);
}

static auto set_ucontext = &setcontext;

static RUA_NO_INLINE void
swap_ucontext(ucontext_t *oucp, const ucontext_t *ucp) {
	volatile char v;
	oucp->_sp = &v;
	::swapcontext(oucp, ucp);
}

inline void make_ucontext(
	ucontext_t *ucp,
	void (*func)(generic_word),
	generic_word func_param,
	bytes_ref stack) {
	ucp->uc_link = nullptr;
	ucp->uc_stack.ss_sp = stack.data();
	ucp->uc_stack.ss_size = stack.size();
	::makecontext(ucp, reinterpret_cast<void (*)()>(func), 1, func_param);
}

} // namespace rua

#endif // ifdef RUA_USING_NATIVE_UCONTEXT

#endif
