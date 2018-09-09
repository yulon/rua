#ifndef _RUA_PROC_HPP
#define _RUA_PROC_HPP

#include "macros.hpp"

#if defined(_WIN32)
	#include "proc/win32.hpp"
	namespace rua {
		using proc = win32::proc;
	}
//#elif defined(RUA_UNIX)
	//#include "proc/unix.hpp"
#else
	#error rua::proc: not supported this platform!
#endif

#endif
