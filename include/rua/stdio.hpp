#ifndef _RUA_STDIO_HPP
#define _RUA_STDIO_HPP

#include "io.hpp"
#include "macros.hpp"

#include <memory>

#if defined(_WIN32)

#include "windows.h"

namespace rua {

inline writer_i get_stdout() {
	auto h = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!h) {
		return nullptr;
	}
	return std::make_shared<sys_stream>(h, false);
}

inline writer_i get_stderr() {
	auto h = GetStdHandle(STD_ERROR_HANDLE);
	if (!h) {
		return nullptr;
	}
	return std::make_shared<sys_stream>(h, false);
}

inline reader_i get_stdin() {
	auto h = GetStdHandle(STD_INPUT_HANDLE);
	if (!h) {
		return nullptr;
	}
	return std::make_shared<sys_stream>(h, false);
}

} // namespace rua

#elif defined(RUA_UNIX)

namespace rua {

inline writer_i get_stdout() {
	return std::make_shared<sys_stream>(STDOUT_FILENO, false);
}

inline writer_i get_stderr() {
	return std::make_shared<sys_stream>(STDERR_FILENO, false);
}

inline reader_i get_stdin() {
	return std::make_shared<sys_stream>(STDIN_FILENO, false);
}

} // namespace rua

#else

#include <cstdio>

namespace rua {

inline writer_i get_stdout() {
	return std::make_shared<sys_stream>(stdout, false);
}

inline writer_i get_stderr() {
	return std::make_shared<sys_stream>(stderr, false);
}

inline reader_i get_stdin() {
	return std::make_shared<sys_stream>(stdin, false);
}

} // namespace rua

#endif

#endif
