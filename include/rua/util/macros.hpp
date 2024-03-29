#ifndef _rua_macros_hpp
#define _rua_macros_hpp

#define RUA_CPP_23 202303L
#define RUA_CPP_20 202002L
#define RUA_CPP_17 201703L
#define RUA_CPP_14 201402L
#define RUA_CPP_11 201103L
#define RUA_CPP_98 199711L

#ifdef _MSC_VER
#if __cplusplus > RUA_CPP_98
#define RUA_CPP __cplusplus
#elif defined(_MSVC_LANG)
#define RUA_CPP _MSVC_LANG
#else
#define RUA_CPP RUA_CPP_98
#endif
#else
#define RUA_CPP __cplusplus
#endif

#ifdef __has_include
#define RUA_HAS_INC(header_name) __has_include(header_name)
#else
#define RUA_HAS_INC(header_name) 0
#endif

#if RUA_HAS_INC(<version>)
#include <version>
#endif

#ifdef __cpp_constexpr
#if __cpp_constexpr >= 200704
#define RUA_HAS_CONSTEXPR
#endif

#if __cpp_constexpr >= 201304
#define RUA_HAS_CONSTEXPR_14
#endif

#if __cpp_constexpr >= 201603
#define RUA_HAS_CONSTEXPR_LM
#endif
#endif

#ifdef RUA_HAS_CONSTEXPR
#define RUA_CONSTEXPR constexpr
#else
#define RUA_CONSTEXPR
#endif

#ifdef RUA_HAS_CONSTEXPR_14
#define RUA_CONSTEXPR_14 constexpr
#else
#define RUA_CONSTEXPR_14
#endif

#ifdef RUA_HAS_CONSTEXPR_LM
#define RUA_CONSTEXPR_LM constexpr
#else
#define RUA_CONSTEXPR_LM
#endif

#ifdef __cpp_if_constexpr
#define RUA_HAS_CONSTEXPR_IF
#define RUA_CONSTEXPR_IF constexpr

#define RUA_HAS_CONSTEXPR_17
#define RUA_CONSTEXPR_17 constexpr
#else
#define RUA_CONSTEXPR_IF

#define RUA_CONSTEXPR_17
#endif

#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606

#define RUA_HAS_INLINE_VAR

// global constexpr variable
#define RUA_CVAL inline constexpr

// global variable with different addresses
#define RUA_CVAR inline

// global inline variable
#define RUA_GVAR(type, name, init_declarator) inline type name init_declarator

#else

// global constexpr variable
#define RUA_CVAL static constexpr

// global variable with different addresses
#define RUA_CVAR static

// global inline variable
#define RUA_GVAR(type, name, init_declarator)                                  \
	inline type &_##name##_instance() {                                        \
		static type instance init_declarator;                                  \
		return instance;                                                       \
	}                                                                          \
	static type &name = name##_instance()

#endif

#define RUA_A(...) __VA_ARGS__
#define RUA_S(s) #s
#define RUA_M2S(m) RUA_S(m)

#if defined(__cpp_static_assert) && __cpp_static_assert >= 201411
#define RUA_SASSERT(...) static_assert(__VA_ARGS__)
#elif RUA_CPP >= RUA_CPP_11
#define RUA_SASSERT(...) static_assert(__VA_ARGS__, #__VA_ARGS__)
#endif

#ifdef __has_cpp_attribute
#if __has_cpp_attribute(fallthrough)
#define RUA_FALLTHROUGH [[fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#define RUA_FALLTHROUGH [[gnu::fallthrough]]
#elif __has_cpp_attribute(clang::fallthrough)
#define RUA_FALLTHROUGH [[clang::fallthrough]]
#else
#define RUA_FALLTHROUGH
#endif
#elif RUA_CPP >= RUA_CPP_17
#define RUA_FALLTHROUGH [[fallthrough]]
#else
#define RUA_FALLTHROUGH
#endif

#if defined(__unix__) || defined(unix) || defined(__unix) ||                   \
	defined(_XOPEN_SOURCE) || defined(_POSIX_SOURCE) ||                        \
	(defined(__APPLE__) && defined(__MACH__))

#define RUA_UNIX

#if defined(__APPLE__) && defined(__MACH__)

#define RUA_DARWIN

#ifdef __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__
#define RUA_IOS
#else
#define RUA_MACOS
#endif

#endif

#endif

#if defined(_AMD64_) || (defined(_M_AMD64_) && _M_AMD64_ == 100) ||            \
	(defined(_M_X64_) && _M_X64_ == 100) || defined(__x86_64) ||               \
	defined(__x86_64__) || defined(__amd64__) || defined(__amd64) ||           \
	defined(_WIN64)

#define RUA_X86 64

#elif defined(_X86_) || (defined(_M_IX86) && _M_IX86 == 600) ||                \
	defined(i386) || defined(__i386) || defined(__i386__) ||                   \
	defined(__i486__) || defined(__i586__) || defined(__i686__) ||             \
	defined(__i686) || defined(__I86__) || defined(__THW_INTEL__) ||           \
	defined(__INTEL__) || defined(_WIN32)

#define RUA_X86 32

#endif

#if defined(_WIN64) ||                                                         \
	(defined(RUA_X86) && RUA_X86 == 64 && defined(__CYGWIN__))
#define RUA_MS64_FASTCALL
#endif

#ifndef _MSC_VER
#define RUA_REGPARM(n) __attribute__((regparm(n)))
#else
#define RUA_REGPARM(n)
#endif

#if defined(__arm64) || defined(_M_ARM64) || defined(_M_ARM64)

#define RUA_ARM 64

#elif defined(__arm__) || defined(__thumb__) || defined(__TARGET_ARCH_ARM) ||  \
	defined(__TARGET_ARCH_THUMB) || defined(_M_ARM)

#define RUA_ARM 32

#endif

#ifdef _MSC_VER
#define RUA_FORCE_INLINE inline __forceinline
#elif defined(__GNUC__)
#define RUA_FORCE_INLINE inline __attribute__((always_inline))
#else
#define RUA_FORCE_INLINE inline
#endif

#ifdef _MSC_VER
#define RUA_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__)
#define RUA_NO_INLINE __attribute__((noinline))
#endif

#if defined(__cpp_impl_coroutine) || defined(__cpp_lib_coroutine)
#define RUA_HAS_AWAIT
#endif

#if defined(__cpp_rtti) && __cpp_rtti
#define RUA_HAS_RTTI __cpp_rtti
#elif defined(_HAS_STATIC_RTTI) && _HAS_STATIC_RTTI
#define RUA_HAS_RTTI _HAS_STATIC_RTTI
#endif

#if defined(__cpp_exceptions) && __cpp_exceptions
#define RUA_HAS_EXCEPTIONS __cpp_exceptions
#define RUA_NOEXCEPT noexcept
#define RUA_NOEXCEPT_IF(cond) noexcept(cond)
#else
#define RUA_NOEXCEPT
#define RUA_NOEXCEPT_IF(cond)
#endif

#define RUA_DI(T, ...)                                                         \
	([&]() -> T {                                                              \
		T $;                                                                   \
		__VA_ARGS__;                                                           \
		return $;                                                              \
	}())

#ifdef __cpp_binary_literals

#define RUA_B(n) (0b##n)

#else

namespace rua {

inline constexpr int _b_non_lowest(int n, int off);

inline constexpr int _b_non_zero(int n, int off) {
	return n ? _b_non_lowest(n, off) : 0;
}

inline constexpr int _b_non_lowest(int n, int off) {
	return (n & 1) << off | _b_non_zero(n / 10, off + 1);
}

inline constexpr int _b(int n) {
	return (n & 1) | _b_non_lowest(n / 10, 1);
}

} // namespace rua

#define RUA_B(n) (rua::_b(n))

#endif

#endif
