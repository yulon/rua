#ifndef _RUA_STDIO_POSIX_HPP
#define _RUA_STDIO_POSIX_HPP

#include "../io/sys_stream/posix.hpp"
#include "../macros.hpp"

#include <unistd.h>

namespace rua { namespace posix {

namespace _stdio {

class _stdio_stream;

using stdout_stream = _stdio_stream;
using stderr_stream = _stdio_stream;
using stdin_stream = _stdio_stream;

class _stdio_stream : public sys_stream {
public:
	_stdio_stream &operator=(sys_stream s) {
		if (!s) {
			close();
			return *this;
		}
		::dup2(s.native_handle(), native_handle());
		return *this;
	}

private:
	constexpr _stdio_stream(int fd) : sys_stream(fd) {}
	_stdio_stream(const _stdio_stream &) = delete;
	_stdio_stream(_stdio_stream &&) = delete;

	friend stdout_stream &out();
	friend stderr_stream &err();
	friend stdin_stream &in();
};

RUA_FORCE_INLINE stdout_stream &out() {
	static stdout_stream s(STDOUT_FILENO);
	return s;
}

RUA_FORCE_INLINE stderr_stream &err() {
	static stderr_stream s(STDERR_FILENO);
	return s;
}

RUA_FORCE_INLINE stdin_stream &in() {
	static stdin_stream s(STDIN_FILENO);
	return s;
}

RUA_FORCE_INLINE stdout_stream &sout() {
	return out();
}

RUA_FORCE_INLINE stderr_stream &serr() {
	return err();
}

RUA_FORCE_INLINE stdin_stream &sin() {
	return in();
}

} // namespace _stdio

using namespace _stdio;

}} // namespace rua::posix

#endif
