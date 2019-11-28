#ifndef RUA_CODE

#ifdef __GNUC__

#if defined(__APPLE__) && defined(__MACH__)
#define _RUA_CODE_SEG __attribute__((section("__TEXT,__rua_text")))
#else
#define _RUA_CODE_SEG __attribute__((section(".text$_ZN3rua10text")))
#endif

#define RUA_CODE(name) static const unsigned char name[] _RUA_CODE_SEG

#elif defined(_MSC_VER)

#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606
#define RUA_CODE(name) inline const unsigned char name[]
#else
#define RUA_CODE(name) static const unsigned char name[]
#endif

#pragma const_seg(push, _rua_code_seg, ".rua")

#endif

#ifndef RUA_CODE_FN

#define RUA_CODE_FN(ret, name, params, code)                                   \
	static ret(*name) params = reinterpret_cast<ret(*) params>(                \
		reinterpret_cast<uintptr_t>(&code[0]));

#endif

#else

#undef RUA_CODE

#ifdef __GNUC__

#undef _RUA_CODE_SEG

#elif defined(_MSC_VER)

#pragma const_seg(pop, _rua_code_seg)
#pragma comment(linker, "/SECTION:.rua,ER")

#endif

#endif
