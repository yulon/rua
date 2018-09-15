#ifndef _RUA_IO_SYS_HPP
#define _RUA_IO_SYS_HPP

#include "c.hpp"

#if defined(_WIN32)
	#include "win32.hpp"
	namespace rua { namespace io {
		namespace sys = win32;
	}}
#else
	namespace rua { namespace io {
		namespace sys = c;
	}}
#endif

#endif
