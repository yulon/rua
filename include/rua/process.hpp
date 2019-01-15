#ifndef _RUA_PROC_HPP
#define _RUA_PROC_HPP

#include "macros.hpp"

#if defined(_WIN32)
	#include "process/win32.hpp"
	namespace rua {
		using process = win32::process;
	}
//#elif defined(RUA_UNIX)
	//#include "process/unix.hpp"
#else
	#error rua::process: not supported this platform!
#endif

#endif
