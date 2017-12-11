#ifndef _TMD_PREDEF_HPP
#define _TMD_PREDEF_HPP

#ifdef _MSC_VER
	#if _MSC_VER >= 1910
		#define TMD_CPP 17
	#elif _MSC_VER >= 1900
		#define TMD_CPP 14
	#elif _MSC_VER >= 1800
		#define TMD_CPP 11
	#else
		#define TMD_CPP 98
	#endif
#else
	#if __cplusplus >= 201703L
		#define TMD_CPP 17
	#elif __cplusplus >= 201402L
		#define TMD_CPP 14
	#elif __cplusplus >= 201103L
		#define TMD_CPP 11
	#elif __cplusplus >= 199711L
		#define TMD_CPP 98
	#else
		#define TMD_CPP 1
	#endif
#endif

#ifdef __cpp_constexpr
	#if __cpp_constexpr >= 200704
		#define TMD_CONSTEXPR_SUPPORTED
	#endif
	#if __cpp_constexpr >= 201304
		#define TMD_CONSTEXPR_14_SUPPORTED
	#endif
	#if __cpp_constexpr >= 201603
		#define TMD_CONSTEXPR_LM_SUPPORTED
	#endif
#endif

#ifdef TMD_CONSTEXPR_SUPPORTED
	#define TMD_CONSTEXPR constexpr
#else
	#define TMD_CONSTEXPR
#endif

#ifdef TMD_CONSTEXPR_14_SUPPORTED
	#define TMD_CONSTEXPR_14 constexpr
#else
	#define TMD_CONSTEXPR_14
#endif

#ifdef TMD_CONSTEXPR_LM_SUPPORTED
	#define TMD_CONSTEXPR_LM constexpr
#else
	#define TMD_CONSTEXPR_LM
#endif

#ifdef __cpp_if_constexpr
	#define TMD_CONSTEXPR_IF_SUPPORTED
#endif

#ifdef TMD_CONSTEXPR_IF_SUPPORTED
	#define TMD_CONSTEXPR_IF constexpr
#else
	#define TMD_CONSTEXPR_IF
#endif

#ifdef TMD_CONSTEXPR_IF_SUPPORTED
	#define TMD_CONSTEXPR_17_SUPPORTED
#endif

#ifdef TMD_CONSTEXPR_17_SUPPORTED
	#define TMD_CONSTEXPR_17 constexpr
#else
	#define TMD_CONSTEXPR_17
#endif

#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606
	#define TMD_INLINE_VAR inline
#else
	#define TMD_INLINE_VAR
#endif

#if defined(__cpp_static_assert) && __cpp_static_assert >= 201411
	#define TMD_STATIC_ASSERT(cond) static_assert(cond)
#elif TMD_CPP >= 11
	#define TMD_STATIC_ASSERT(cond) static_assert(cond, #cond)
#else
	#define TMD_STATIC_ASSERT(cond) assert(cond)
#endif

#endif
