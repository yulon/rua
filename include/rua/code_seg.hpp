#ifndef RUA_CODE_SEG

#ifdef __GNUC__

#if defined(__APPLE__) && defined(__MACH__)
#define RUA_CODE_SEG __attribute__((section("__TEXT,__rua_text")))
#else
#define RUA_CODE_SEG __attribute__((section(".text._ZN3rua10text")))
#endif

#define RUA_CODE(name) static RUA_CODE_SEG const unsigned char name[]

#elif defined(_MSC_VER)

#include "macros.hpp"

#pragma section(".text")

#define RUA_CODE_SEG __declspec(allocate(".text"))

#define RUA_CODE(name) RUA_CVAR RUA_CODE_SEG const unsigned char name[]

#endif

#define RUA_CODE_FN(ret, name, params, code)                                   \
	static ret(*name) params = reinterpret_cast<ret(*) params>(                \
		reinterpret_cast<uintptr_t>(&code[0]));

#endif
