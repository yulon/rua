#ifndef _RUA_STDIO_C_HPP
#define _RUA_STDIO_C_HPP

#include "../io/c_stream.hpp"
#include "../macros.hpp"

#include <cstdio>

namespace rua { namespace c {

namespace _stdio {

using stdout_stream = c_stream;
using stderr_stream = c_stream;
using stdin_stream = c_stream;

inline stdout_stream &out() {
	static stdout_stream s(stdout, false);
	return s;
}

inline stderr_stream &err() {
	static stderr_stream s(stderr, false);
	return s;
}

inline stdin_stream &in() {
	static stdin_stream s(stdin, false);
	return s;
}

inline stdout_stream &sout() {
	return out();
}

inline stderr_stream &serr() {
	return err();
}

inline stdin_stream &sin() {
	return in();
}

} // namespace _stdio

using namespace _stdio;

}} // namespace rua::c

#endif
