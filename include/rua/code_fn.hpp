#ifndef _RUA_CODE_FN_HPP
#define _RUA_CODE_FN_HPP

#include "byte.hpp"
#include "macros.hpp"

#ifndef __GNUC__
#include "memory.hpp"
#endif

#include <cstdint>

namespace rua {

#ifdef __GNUC__

#if defined(__APPLE__) && defined(__MACH__)
#define _RUA_EXECUTABLE_CODE __attribute__((section("__TEXT,__rua_text")))
#else
#define _RUA_EXECUTABLE_CODE __attribute__((section(".text$_ZN3rua10text")))
#endif

#define RUA_CODE(name) static const byte name[] _RUA_EXECUTABLE_CODE

#define RUA_CODE_FN(ret, name, params, args, code)                             \
	static ret(*name) params = reinterpret_cast<ret(*) params>(                \
		reinterpret_cast<uintptr_t>(&code[0]));

#else

template <typename T, size_t N>
RUA_FORCE_INLINE T _to_executable_code(const byte (&code)[N]) {
	mem_chmod(&code[0], N, mem_read | mem_exec);
	return reinterpret_cast<T>(reinterpret_cast<uintptr_t>(&code[0]));
}

#define RUA_CODE(name) RUA_MULTIDEF_VAR const byte name[]

#define RUA_CODE_FN(ret, name, params, args, code)                             \
	using _##name##_t = ret(*) params;                                         \
	RUA_FORCE_INLINE _##name##_t _##name() {                                   \
		static _##name##_t f = _to_executable_code<_##name##_t>(code);         \
		return f;                                                              \
	}                                                                          \
	static _##name##_t name = _##name();

#endif

} // namespace rua

#endif
