#ifndef _RUA_PIPE_HPP
#define _RUA_PIPE_HPP

#include "macros.hpp"

#if defined(_WIN32)
	#include "pipe/win32.hpp"
//#elif defined(RUA_UNIX)
	//#include "pipe/unix.hpp"
#else
	#error rua::os::pipe: not supported this platform!
#endif

#endif
