#ifndef _RUA_CP_TLS_HPP
#define _RUA_CP_TLS_HPP

#include "../macros.hpp"

#if defined(_WIN32)
	#include "tls/win32.hpp"
#elif defined(RUA_UNIX)
	#include "tls/posix.hpp"
#else
	#error rua::cp::tls: not supported this platform!
#endif

#endif
