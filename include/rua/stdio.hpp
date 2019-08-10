#ifndef _RUA_STDIO_HPP
#define _RUA_STDIO_HPP

#include "io.hpp"

#if defined(_WIN32)

#include "windows.h"

namespace rua {

inline sys_stream get_stdout() {
	return sys_stream(GetStdHandle(STD_OUTPUT_HANDLE), false);
}

inline sys_stream get_stderr() {
	return sys_stream(GetStdHandle(STD_ERROR_HANDLE), false);
}

inline sys_stream get_stdin() {
	return sys_stream(GetStdHandle(STD_INPUT_HANDLE), false);
}

} // namespace rua

#else

#include <cstdio>

namespace rua {

inline sys_stream get_stdout() {
	return sys_stream(stdout, false);
	return inst;
}

inline sys_stream get_stderr() {
	return sys_stream(stderr, false);
	return inst;
}

inline sys_stream get_stdin() {
	return sys_stream(stdin, false);
}

} // namespace rua

#endif

#endif
