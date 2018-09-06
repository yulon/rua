#ifndef _RUA_FILE_HPP
#define _RUA_FILE_HPP

#if defined(_WIN32)
	#include "file/win32.hpp"
	namespace rua {
		using file = win32::file;
	}
#else
	#include "file/c.hpp"
	namespace rua {
		using file = c::file;
	}
#endif

#endif
