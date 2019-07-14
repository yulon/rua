#ifndef _RUA_IO_SYS_STREAM_HPP
#define _RUA_IO_SYS_STREAM_HPP

#ifdef _WIN32
	#include "sys_stream/win32.hpp"
	namespace rua {
		using sys_stream = win32::sys_stream;
	}
#else
	#include "c_stream.hpp"
	namespace rua {
		using sys_stream = c_stream;
	}
#endif

#endif
