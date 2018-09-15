#ifndef _RUA_STDIO_HPP
#define _RUA_STDIO_HPP

#include "io.hpp"

#if defined(_WIN32)
	#include "windows.h"

	namespace rua {
		inline io::sys::writer get_stdout() {
			return GetStdHandle(STD_OUTPUT_HANDLE);
		}

		inline io::sys::writer get_stderr() {
			return GetStdHandle(STD_ERROR_HANDLE);
		}

		inline io::sys::reader get_stdin() {
			return GetStdHandle(STD_INPUT_HANDLE);
		}
	}
#else
	#include <cstdio>

	namespace rua {
		inline io::sys::writer get_stdout() {
			return stdout;
		}

		inline io::sys::writer get_stderr() {
			return stderr;
		}

		inline io::sys::reader get_stdin() {
			return stdin;
		}
	}
#endif

#endif
