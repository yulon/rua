#ifndef _TMD_PREDEF_HPP
#define _TMD_PREDEF_HPP

#if defined(_MSC_VER)
	#if _MSC_VER >= 2000
		#define TMD_CPP 17
	#elif _MSC_VER >= 1900
		#define TMD_CPP 14
	#elif _MSC_VER >= 1700
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

#if TMD_CPP >= 17
	#define TMD_CONSTEXPR17 constexpr
#else
	#define TMD_CONSTEXPR17
#endif

#if TMD_CPP >= 14
	#define TMD_CONSTEXPR14 constexpr
#else
	#define TMD_CONSTEXPR14
#endif

#if TMD_CPP >= 11
	#define TMD_CONSTEXPR constexpr
#else
	#define TMD_CONSTEXPR
#endif

#if TMD_CPP >= 17
	#define TMD_INLINE17 inline
#else
	#define TMD_INLINE17
#endif

#endif
