#ifndef _RUA_CP_CORO_HPP
#define _RUA_CP_CORO_HPP

#include "../macros.hpp"

#if defined(_WIN32)
	#include "coro/win32.hpp"
#elif defined(RUA_UNIX)
	//#include "coro/unix.hpp"
#else
	#error rua::coro: not supported this platform!
#endif

#endif
