#ifndef _rua_sys_stream_posix_hpp
#define _rua_sys_stream_posix_hpp

#include "../../io/util.hpp"
#include "../../util.hpp"

#include <unistd.h>

#include <cassert>

namespace rua { namespace posix {

class sys_stream : public stream_base {
public:
	using native_handle_t = int;

	constexpr sys_stream(native_handle_t fd = -1, bool need_close = true) :
		_fd(fd), _nc(_fd >= 0 ? need_close : false) {}

	template <
		typename NullPtr,
		typename = enable_if_t<is_null_pointer<NullPtr>::value>>
	constexpr sys_stream(NullPtr) : sys_stream() {}

	sys_stream(const sys_stream &src) {
		if (!src._fd) {
			_fd = -1;
			return;
		}
		if (!src._nc) {
			_fd = src._fd;
			_nc = false;
			return;
		}
		_fd = ::dup(src._fd);
		assert(_fd >= 0);
		_nc = true;
	}

	sys_stream(sys_stream &&src) : sys_stream(src._fd, src._nc) {
		src.detach();
	}

	RUA_OVERLOAD_ASSIGNMENT(sys_stream)

	virtual ~sys_stream() {
		close();
	}

	native_handle_t &native_handle() {
		return _fd;
	}

	native_handle_t native_handle() const {
		return _fd;
	}

	virtual operator bool() const {
		return _fd >= 0;
	}

	virtual ssize_t read(bytes_ref buf) {
		assert(*this);

		return _read(_fd, buf);
	}

	virtual ssize_t write(bytes_view data) {
		assert(*this);

		return _write(_fd, data);
	}

	bool is_need_close() const {
		return _fd >= 0 && _nc;
	}

	virtual void close() {
		if (_fd < 0) {
			return;
		}
		if (_nc) {
			::close(_fd);
		}
		_fd = -1;
	}

	void detach() {
		_nc = false;
	}

	sys_stream dup() const {
		if (_fd < 0) {
			return nullptr;
		}
		return ::dup(_fd);
	}

private:
	int _fd;
	bool _nc;

	static ssize_t _read(int _fd, bytes_ref p) {
		return static_cast<ssize_t>(::read(_fd, p.data(), p.size()));
	}

	static ssize_t _write(int _fd, bytes_view p) {
		return static_cast<ssize_t>(::write(_fd, p.data(), p.size()));
	}
};

}} // namespace rua::posix

#endif
