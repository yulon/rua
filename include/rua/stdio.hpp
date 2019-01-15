#ifndef _RUA_STDIO_HPP
#define _RUA_STDIO_HPP

#include "io.hpp"
#include "str.hpp"

#if defined(_WIN32)
	#include "windows.h"

	namespace rua {
		inline io::writer_i stdout_writer() {
			return io::os_stream(GetStdHandle(STD_OUTPUT_HANDLE), false);
		}

		inline io::writer_i stderr_writer() {
			return io::os_stream(GetStdHandle(STD_ERROR_HANDLE), false);
		}

		inline io::reader_i stdin_reader() {
			return io::os_stream(GetStdHandle(STD_INPUT_HANDLE), false);
		}
	}
#else
	#include <cstdio>

	namespace rua {
		inline io::writer_i stdout_writer() {
			return io::os_stream(stdout, false);
		}

		inline io::writer_i stderr_writer() {
			return io::os_stream(stderr, false);
		}

		inline io::reader_i stdin_reader() {
			return io::os_stream(stdin, false);
		}
	}
#endif

#endif
