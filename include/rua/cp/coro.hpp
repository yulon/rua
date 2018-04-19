#ifndef _RUA_CP_CORO_HPP
#define _RUA_CP_CORO_HPP

#if defined(_WIN32)
	#include "coro/win32.hpp"
#else
	//#include "coro/uni.hpp"
	#error rua::process: not supported this platform!
#endif

#endif
