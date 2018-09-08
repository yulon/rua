#ifndef _RUA_FILE_HPP
#define _RUA_FILE_HPP

#include "file/c.hpp"

#if defined(_WIN32)
	#include "file/win32.hpp"
	namespace rua {
		using file = win32::file;
	}
#else
	namespace rua {
		using file = c::file;
	}
#endif

#endif
