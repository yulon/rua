#ifndef _RUA_STDIO_HPP
#define _RUA_STDIO_HPP

#include "io.hpp"

#if defined(_WIN32)

#include "windows.h"

namespace rua {

inline writer_i stdout_writer() {
	return sys_stream(GetStdHandle(STD_OUTPUT_HANDLE), false);
}

inline writer_i stderr_writer() {
	return sys_stream(GetStdHandle(STD_ERROR_HANDLE), false);
}

inline reader_i stdin_reader() {
	return sys_stream(GetStdHandle(STD_INPUT_HANDLE), false);
}

} // namespace rua

#else

#include <cstdio>

namespace rua {

inline writer_i stdout_writer() {
	return sys_stream(stdout, false);
}

inline writer_i stderr_writer() {
	return sys_stream(stderr, false);
}

inline reader_i stdin_reader() {
	return sys_stream(stdin, false);
}

} // namespace rua

#endif

#endif
