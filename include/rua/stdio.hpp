#ifndef _RUA_STDIO_HPP
#define _RUA_STDIO_HPP

#include "io.hpp"

#if defined(_WIN32)
	#include "windows.h"

	namespace rua {
		inline io::sys::writer stdout_writer() {
			return GetStdHandle(STD_OUTPUT_HANDLE);
		}

		inline io::sys::writer stderr_writer() {
			return GetStdHandle(STD_ERROR_HANDLE);
		}

		inline io::sys::reader stdin_reader() {
			return GetStdHandle(STD_INPUT_HANDLE);
		}
	}
#else
	#include <cstdio>

	namespace rua {
		inline io::sys::writer stdout_writer() {
			return stdout;
		}

		inline io::sys::writer stderr_writer() {
			return stderr;
		}

		inline io::sys::reader stdin_reader() {
			return stdin;
		}
	}
#endif

#endif
