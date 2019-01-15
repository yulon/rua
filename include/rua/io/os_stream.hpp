#ifndef _RUA_IO_SYS_HPP
#define _RUA_IO_SYS_HPP

#include "c_stream.hpp"

#if defined(_WIN32)
	#include "win32_stream.hpp"
	namespace rua { namespace io {
		using os_stream = win32_stream;
	}}
#else
	namespace rua { namespace io {
		using os_stream = c_stream;
	}}
#endif

#endif
