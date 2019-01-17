#ifndef _RUA_CONTEXT_HPP
#define _RUA_CONTEXT_HPP

#include "macros.hpp"

#if defined(RUA_AMD64) || defined(RUA_I386)

#include "any_word.hpp"
#include "mem/protect.hpp"
#include "bin.hpp"

#include <cstdint>
#include <type_traits>

namespace rua {

/*
	Reference from
		https://github.com/skywind3000/collection/tree/master/ucontext
		https://github.com/hnes/libaco/blob/master/acosw.S
		https://github.com/boostorg/ucontext/tree/develop/src/asm
*/

struct mcontext_t {
	#if defined(RUA_AMD64) || defined(RUA_I386)
		uintptr_t a; // 0
		uintptr_t b; // 4, 8
		uintptr_t c; // 8, 16
		uintptr_t d; // 12, 24
		uintptr_t si; // 16, 32
		uintptr_t di; // 20, 40
		uintptr_t sp; // 24, 48
		uintptr_t bp; // 28, 56
		uintptr_t caller_ip; // 32, 64
		uintptr_t flags; // 36, 72

		#ifdef RUA_AMD64
			uintptr_t r8; // 80
			uintptr_t r9; // 88
			uintptr_t r10; // 96
			uintptr_t r11; // 104
			uintptr_t r12; // 112
			uintptr_t r13; // 120
			uintptr_t r14; // 128
			uintptr_t r15; // 136

			uintptr_t &caller_param() {
				return
					#ifdef RUA_MS64_FASTCALL
						c
					#else
						di
					#endif
				;
			}

			uintptr_t caller_param() const {
				return
					#ifdef RUA_MS64_FASTCALL
						c
					#else
						di
					#endif
				;
			}
		#endif

		#ifdef RUA_I386
			uintptr_t &caller_param() {
				return reinterpret_cast<uintptr_t *>(sp)[2];
			}

			uintptr_t caller_param() const {
				return reinterpret_cast<const uintptr_t *>(sp)[2];
			}
		#endif

		uint32_t mxcsr; // 40, 144
		uint16_t fcw; // 44, 148
		uint16_t _padding;
	#endif
};
#ifdef RUA_AMD64
	RUA_SASSERT(sizeof(mcontext_t) == 152);
#elif RUA_I386
	RUA_SASSERT(sizeof(mcontext_t) == 48);
#endif

struct oscontext_t {
	#ifdef _WIN32
		uintptr_t deallocation_stack; // 48, 152
		uintptr_t stack_limit; // 52, 160
		uintptr_t stack_base; // 56, 168
		uintptr_t exception_list; // 60, 176
		uintptr_t fls; // 64, 184
	#else
		#define _RUA_EMPTY_OSCONTEXT
	#endif
};

struct ucontext_t {
	mcontext_t mcontext;

	#ifndef _RUA_EMPTY_OSCONTEXT
		oscontext_t oscontext;
	#endif
};
RUA_SASSERT(std::is_trivial<ucontext_t>::value);

template <typename CodeBytes>
inline any_ptr _code_init(CodeBytes &&cb) {
	mem::protect(&cb, sizeof(cb), mem::protect_read | mem::protect_exec);
	return &cb;
}

inline bool get_ucontext(ucontext_t *ucp) {
	static const uint8_t code[]{
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

	static bool (*fn)(ucontext_t *) = _code_init(code);

	return fn(ucp);
}

inline void set_ucontext(const ucontext_t *ucp) {
	static const uint8_t code[]{
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

	static void (*fn)(const ucontext_t *) = _code_init(code);

	fn(ucp);
}

inline void swap_ucontext(ucontext_t *oucp, const ucontext_t *ucp) {
	static const uint8_t code[]{
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

	static void (*fn)(ucontext_t *, const ucontext_t *) = _code_init(code);

	fn(oucp, ucp);
}

inline void make_ucontext(ucontext_t *ucp, void (*func)(any_word), any_word func_param, bin_ref stack) {
	auto stk_end = (stack.base() + stack.size()).value();
	ucp->mcontext.sp = stk_end - 3 * sizeof(uintptr_t);
	ucp->mcontext.caller_param() = func_param;
	ucp->mcontext.caller_ip = reinterpret_cast<uintptr_t>(func);

	#ifdef _WIN32
		ucp->oscontext.stack_limit = stack.base().value();
		ucp->oscontext.stack_base = stk_end - 1;
	#endif
}

}

#else

#include <ucontext.h>

namespace rua {

using ucontext_t = ::ucontext_t;

inline bool get_ucontext(ucontext_t *ucp) {
	return !getcontext(ucp);
}

inline void set_ucontext(const ucontext_t *ucp) {
	setcontext(ucp);
}

inline void swap_ucontext(ucontext_t *oucp, const ucontext_t *ucp) {
	swapcontext(oucp, ucp);
}

inline void make_ucontext(ucontext_t *ucp, void (*func)(any_word), any_word func_param, bin_ref stack) {
	ucp->uc_link = nullptr;
	ucp->uc_stack.ss_sp = stack.base();
	ucp->uc_stack.ss_size = stack.size();
	makecontext(ucp, func, 1, func_param);
}

}

#endif

#endif
