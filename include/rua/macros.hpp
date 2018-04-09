#ifndef _RUA_MACROS_HPP
#define _RUA_MACROS_HPP

#ifdef _MSC_VER
	#ifdef _MSVC_LANG
		#if _MSVC_LANG >= 201703L
			#define RUA_CPP 17
		#elif _MSVC_LANG >= 201402L
			#define RUA_CPP 14
		#elif _MSVC_LANG >= 201103L
			#define RUA_CPP 11
		#elif _MSVC_LANG >= 199711L
			#define RUA_CPP 98
		#else
			#define RUA_CPP 1
		#endif
	#else
		#if _MSC_VER >= 1910
			#define RUA_CPP 17
		#elif _MSC_VER >= 1900
			#define RUA_CPP 14
		#elif _MSC_VER >= 1800
			#define RUA_CPP 11
		#else
			#define RUA_CPP 98
		#endif
	#endif
#else
	#if __cplusplus >= 201703L
		#define RUA_CPP 17
	#elif __cplusplus >= 201402L
		#define RUA_CPP 14
	#elif __cplusplus >= 201103L
		#define RUA_CPP 11
	#elif __cplusplus >= 199711L
		#define RUA_CPP 98
	#else
		#define RUA_CPP 1
	#endif
#endif

#ifdef __cpp_constexpr
	#if __cpp_constexpr >= 200704
		#define RUA_CONSTEXPR_SUPPORTED
	#endif
	#if __cpp_constexpr >= 201304
		#define RUA_CONSTEXPR_14_SUPPORTED
	#endif
	#if __cpp_constexpr >= 201603
		#define RUA_CONSTEXPR_LM_SUPPORTED
	#endif
#endif

#ifdef RUA_CONSTEXPR_SUPPORTED
	#define RUA_CONSTEXPR constexpr
#else
	#define RUA_CONSTEXPR
#endif

#ifdef RUA_CONSTEXPR_14_SUPPORTED
	#define RUA_CONSTEXPR_14 constexpr
#else
	#define RUA_CONSTEXPR_14
#endif

#ifdef RUA_CONSTEXPR_LM_SUPPORTED
	#define RUA_CONSTEXPR_LM constexpr
#else
	#define RUA_CONSTEXPR_LM
#endif

#ifdef __cpp_if_constexpr
	#define RUA_CONSTEXPR_IF_SUPPORTED
#endif

#ifdef RUA_CONSTEXPR_IF_SUPPORTED
	#define RUA_CONSTEXPR_IF constexpr
#else
	#define RUA_CONSTEXPR_IF
#endif

#ifdef RUA_CONSTEXPR_IF_SUPPORTED
	#define RUA_CONSTEXPR_17_SUPPORTED
#endif

#ifdef RUA_CONSTEXPR_17_SUPPORTED
	#define RUA_CONSTEXPR_17 constexpr
#else
	#define RUA_CONSTEXPR_17
#endif

#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606
	#define RUA_INLINE_VAR inline
#else
	#define RUA_INLINE_VAR
#endif

#if defined(__cpp_static_assert) && __cpp_static_assert >= 201411
	#define RUA_STATIC_ASSERT(cond) static_assert(cond)
#elif RUA_CPP >= 11
	#define RUA_STATIC_ASSERT(cond) static_assert(cond, #cond)
#else
	#define RUA_STATIC_ASSERT(cond) assert(cond)
#endif

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
	#define RUA_UNIX

	#if defined(__APPLE__) && defined(__MACH__)
		#define RUA_MAC
	#endif
#endif

#if RUA_CPP >= 17
	#define RUA_FALLTHROUGH [[fallthrough]]
#elif defined(__GNUC__)
	#if RUA_CPP >= 11
		#define RUA_FALLTHROUGH [[gnu::fallthrough]]
	#else
		#define RUA_FALLTHROUGH __attribute__ ((fallthrough))
	#endif
#else
	#define RUA_FALLTHROUGH
#endif

#endif
