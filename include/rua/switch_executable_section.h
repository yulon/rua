#ifndef RUA_EXECUTABLE

#ifdef __GNUC__
#if defined(__APPLE__) && defined(__MACH__)
#define RUA_EXECUTABLE __attribute__((section("__TEXT,__rua_text")))
#else
#define RUA_EXECUTABLE __attribute__((section(".text$_ZN3rua10text")))
#endif
#else
#define RUA_EXECUTABLE
#endif

#ifdef _MSC_VER
#pragma const_seg(push, _rua_switch_executable_section, ".text")
#endif

#else

#undef RUA_EXECUTABLE

#ifdef _MSC_VER
#pragma const_seg(pop, _rua_switch_executable_section)
#endif

#endif
