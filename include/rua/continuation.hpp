#ifndef _RUA_CONTINUATION_HPP
#define _RUA_CONTINUATION_HPP

#include "macros.hpp"

#if !defined(RUA_AMD64) && !defined(RUA_I386)
	#error rua::continuation: not supported this platform!
#endif

#include "any_word.hpp"
#include "mem/protect.hpp"
#include "bin.hpp"

#include <cstdint>

namespace rua {

/*
	Reference from
		https://github.com/skywind3000/collection/tree/master/context
		https://github.com/hnes/libaco/blob/master/acosw.S
		https://github.com/boostorg/context/tree/develop/src/asm
*/

class continuation {
public:
	continuation() = default;

	continuation(std::nullptr_t) : continuation() {}

	continuation(void (*func)(any_word), any_word func_param, bin_ref stack) {
		make(func, func_param, stack);
	}

	bool save() {
		static const uint8_t code[]{
			#ifdef RUA_AMD64
				#ifdef RUA_MS64_FASTCALL
					#include "continuation/save_amd64_win.inc"
				#else
					#include "continuation/save_amd64_sysv.inc"
				#endif
			#elif defined(RUA_I386)
				#ifdef _WIN32
					#include "continuation/save_i386_win.inc"
				#else
					#include "continuation/save_i386.inc"
				#endif
			#endif
		};

		static bool (*fn)(continuation *) = _code_init(code);

		return fn(this);
	}

	void restore() const {
		static const uint8_t code[]{
			#ifdef RUA_AMD64
				#ifdef RUA_MS64_FASTCALL
					#include "continuation/restore_amd64_win.inc"
				#else
					#include "continuation/restore_amd64_sysv.inc"
				#endif
			#elif defined(RUA_I386)
				#ifdef _WIN32
					#include "continuation/restore_i386_win.inc"
				#else
					#include "continuation/restore_i386.inc"
				#endif
			#endif
		};

		static void (*fn)(const continuation *) = _code_init(code);

		fn(this);
	}

	/*
		// For reference only.
		void restore(continuation &cur_cont_saver) const {
			if (cur_cont_saver.save()) {
				restore();
			}
		}
	*/

	void restore(continuation &cur_cont_saver) const {
		static const uint8_t code[]{
			#ifdef RUA_AMD64
				#ifdef RUA_MS64_FASTCALL
					#include "continuation/restore_and_save_amd64_win.inc"
				#else
					#include "continuation/restore_and_save_amd64_sysv.inc"
				#endif
			#elif defined(RUA_I386)
				#ifdef _WIN32
					#include "continuation/restore_and_save_i386_win.inc"
				#else
					#include "continuation/restore_and_save_i386.inc"
				#endif
			#endif
		};

		static void (*fn)(const continuation *, continuation *) = _code_init(code);

		fn(this, &cur_cont_saver);
	}

	void bind(void (*func)(any_word), any_word func_param, bin_ref stack) {
		auto stk_end = (stack.base() + stack.size()).value();
		mctx.sp = stk_end - 3 * sizeof(uintptr_t);
		mctx.caller_param() = func_param;
		mctx.caller_ip = reinterpret_cast<uintptr_t>(func);

		#ifdef _WIN32
			osctx.stack_limit = stack.base().value();
			osctx.stack_base = stk_end - 1;
		#endif
	}

	void make(void (*func)(any_word), any_word func_param, bin_ref stack) {
		save();
		bind(func, func_param, stack);
	}

	////////////////////////////////////////////////////////////////

	struct mctx_t {
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
	} mctx;

	#ifdef _WIN32
		struct osctx_t {
			uintptr_t deallocation_stack; // 48, 152
			uintptr_t stack_limit; // 52, 160
			uintptr_t stack_base; // 56, 168
			uintptr_t exception_list; // 60, 176
			uintptr_t fls; // 64, 184
		} osctx;
	#endif

private:
	template <typename CodeBytes>
	static any_ptr _code_init(CodeBytes &&cb) {
		mem::protect(&cb, sizeof(cb), mem::protect_read | mem::protect_exec);
		return &cb;
	}
};

#ifdef _WIN32
	#ifdef RUA_AMD64
		RUA_SASSERT(offsetof(continuation, osctx) == 152);
	#endif

	#ifdef RUA_I386
		RUA_SASSERT(offsetof(continuation, osctx) == 48);
	#endif
#endif

}

#endif
