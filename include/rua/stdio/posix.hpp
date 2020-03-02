#ifndef _RUA_STDIO_POSIX_HPP
#define _RUA_STDIO_POSIX_HPP

#include "../io/sys_stream.hpp"
#include "../macros.hpp"

#include <unistd.h>

namespace rua { namespace posix {

namespace _stdio {

RUA_FORCE_INLINE sys_stream &out() {
	static sys_stream inst(STDOUT_FILENO, false);
	return inst;
}

RUA_FORCE_INLINE sys_stream &err() {
	static sys_stream inst(STDERR_FILENO, false);
	return inst;
}

RUA_FORCE_INLINE sys_stream &in() {
	static sys_stream inst(STDIN_FILENO, false);
	return inst;
}

RUA_FORCE_INLINE void set_out(const sys_stream &w) {
	if (!w) {
		::close(STDOUT_FILENO);
		return;
	}
	::dup2(w.native_handle(), STDOUT_FILENO);
}

RUA_FORCE_INLINE void set_err(const sys_stream &w) {
	if (!w) {
		::close(STDERR_FILENO);
		return;
	}
	::dup2(w.native_handle(), STDERR_FILENO);
}

RUA_FORCE_INLINE void set_in(const sys_stream &r) {
	if (!r) {
		::close(STDIN_FILENO);
		return;
	}
	::dup2(r.native_handle(), STDIN_FILENO);
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

}} // namespace rua::posix

#endif
