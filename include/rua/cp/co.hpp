#ifndef _RUA_CP_CO_HPP
#define _RUA_CP_CO_HPP

#include "../macros.hpp"

#if defined(_WIN32)
	#include "co/win32.hpp"
#elif defined(RUA_UNIX)
	#include "co/unix.hpp"
#else
	#error rua::co: not supported this platform!
#endif

#endif
