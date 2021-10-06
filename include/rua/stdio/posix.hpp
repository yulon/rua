#ifndef _RUA_STDIO_POSIX_HPP
#define _RUA_STDIO_POSIX_HPP

#include "../macros.hpp"
#include "../sys/stream/posix.hpp"

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
	constexpr _stdio_stream(int fd) : sys_stream(fd, false) {}
	_stdio_stream(const _stdio_stream &) = delete;
	_stdio_stream(_stdio_stream &&) = delete;

	friend stdout_stream &out();
	friend stderr_stream &err();
	friend stdin_stream &in();
};

inline stdout_stream &out() {
	static stdout_stream s(STDOUT_FILENO);
	return s;
}

inline stderr_stream &err() {
	static stderr_stream s(STDERR_FILENO);
	return s;
}

inline stdin_stream &in() {
	static stdin_stream s(STDIN_FILENO);
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

}} // namespace rua::posix

#endif
