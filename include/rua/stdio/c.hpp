#ifndef _RUA_STDIO_C_HPP
#define _RUA_STDIO_C_HPP

#include "../../io/sys_stream.hpp"
#include "../../macros.hpp"

#include <cstdio>

namespace rua { namespace c {

namespace _stdio {

RUA_FORCE_INLINE sys_stream &out() {
	static sys_stream inst(stdout, false);
	return inst;
}

RUA_FORCE_INLINE sys_stream &err() {
	static sys_stream inst(stderr, false);
	return inst;
}

RUA_FORCE_INLINE sys_stream &in() {
	static sys_stream inst(stdin, false);
	return inst;
}

RUA_FORCE_INLINE sys_stream &sout() {
	return out();
}

RUA_FORCE_INLINE sys_stream &serr() {
	return err();
}

RUA_FORCE_INLINE sys_stream &sin() {
	return in();
}

} // namespace _stdio

using namespace _stdio;

}} // namespace rua::c

#endif
