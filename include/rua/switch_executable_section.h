#ifndef RUA_EXECUTABLE

#ifdef _MSC_VER
#define RUA_EXECUTABLE
#else
#define RUA_EXECUTABLE __attribute__((section(".text")))
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
