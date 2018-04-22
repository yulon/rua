#ifndef _RUA_CP_CORO_HPP
#define _RUA_CP_CORO_HPP

#if defined(_WIN32) && !defined(RUA_CP_CORO_USING_UNI_IMPL)
	#include "coro/win32.hpp"
#else
	#include "coro/uni.hpp"
#endif

#endif
