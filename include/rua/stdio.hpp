#ifndef _RUA_STDIO_HPP
#define _RUA_STDIO_HPP

#include "io.hpp"

#if defined(_WIN32)

#include "windows.h"

namespace rua {

inline sys_stream &get_stdout() {
	static sys_stream inst(GetStdHandle(STD_OUTPUT_HANDLE), false);
	return inst;
}

inline sys_stream &get_stderr() {
	static sys_stream inst(GetStdHandle(STD_ERROR_HANDLE), false);
	return inst;
}

inline sys_stream &get_stdin() {
	static sys_stream inst(GetStdHandle(STD_INPUT_HANDLE), false);
	return inst;
}

inline void reset_stdio() {
	get_stdout() = sys_stream(GetStdHandle(STD_OUTPUT_HANDLE), false);
	get_stderr() = sys_stream(GetStdHandle(STD_ERROR_HANDLE), false);
	get_stdin() = sys_stream(GetStdHandle(STD_INPUT_HANDLE), false);
}

} // namespace rua

#else

#include <cstdio>

namespace rua {

inline sys_stream &get_stdout() {
	static sys_stream inst(stdout, false);
	return inst;
}

inline sys_stream &get_stderr() {
	static sys_stream inst(stderr, false);
	return inst;
}

inline sys_stream &get_stdin() {
	static sys_stream inst(stdin, false);
	return inst;
}

inline void reset_stdio() {
	get_stdout() = sys_stream(stdout, false);
	get_stderr() = sys_stream(stderr, false);
	get_stdin() = sys_stream(stdin, false);
}

} // namespace rua

#endif

#endif
