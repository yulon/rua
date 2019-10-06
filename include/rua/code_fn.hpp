#ifndef _RUA_CODE_FN_HPP
#define _RUA_CODE_FN_HPP

#include "bytes.hpp"
#include "macros.hpp"
#include "memory.hpp"

namespace rua {

#ifdef __GNUC__

#if defined(__APPLE__) && defined(__MACH__)
#define _RUA_EXECUTABLE_CODE __attribute__((section("__TEXT,__rua_text")))
#else
#define _RUA_EXECUTABLE_CODE __attribute__((section(".text$_ZN3rua10text")))
#endif

#define _RUA_TO_EXECUTABLE_CODE(code) reinterpret_cast<uintptr_t>(code)

#else

#define _RUA_EXECUTABLE_CODE

template <size_t N>
RUA_FORCE_INLINE uintptr_t _to_executable_code(const byte (&code)[N]) {
	mem_chmod(code, N, mem_read | mem_exec);
	return reinterpret_cast<uintptr_t>(code);
}

#define _RUA_TO_EXECUTABLE_CODE(code) _to_executable_code(code)

#endif

#define RUA_CODE(name) static const byte name[] _RUA_EXECUTABLE_CODE

#define RUA_CODE_FN(ret, name, params, args, code)                             \
	RUA_FORCE_INLINE ret name params {                                         \
		static ret(*fn_ptr) params =                                           \
			reinterpret_cast<ret(*) params>(_RUA_TO_EXECUTABLE_CODE(code));    \
		return fn_ptr args;                                                    \
	}

} // namespace rua

#endif
