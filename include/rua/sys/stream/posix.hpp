#ifndef _RUA_SYS_STREAM_POSIX_HPP
#define _RUA_SYS_STREAM_POSIX_HPP

#include "../../io/util.hpp"
#include "../../macros.hpp"
#include "../../sched.hpp"
#include "../../types/traits.hpp"

#include <unistd.h>

#include <cassert>

namespace rua { namespace posix {

class sys_stream : public read_write_util<sys_stream> {
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

	~sys_stream() {
		close();
	}

	native_handle_t &native_handle() {
		return _fd;
	}

	native_handle_t native_handle() const {
		return _fd;
	}

	explicit operator bool() const {
		return _fd >= 0;
	}

	ptrdiff_t read(bytes_ref p) {
		assert(*this);

		auto dzr = this_dozer();
		if (!dzr->is_own_stack()) {
			auto buf = try_make_heap_buffer(p);
			if (buf) {
				auto sz = await(std::move(dzr), _read, _fd, bytes_ref(buf));
				if (sz > 0) {
					p.copy_from(buf);
				}
				return sz;
			}
		}
		return await(std::move(dzr), _read, _fd, p);
	}

	ptrdiff_t write(bytes_view p) {
		assert(*this);

		auto dzr = this_dozer();
		if (!dzr->is_own_stack()) {
			auto data = try_make_heap_data(p);
			if (data) {
				return await(std::move(dzr), _write, _fd, bytes_view(data));
			}
		}
		return await(std::move(dzr), _write, _fd, p);
	}

	bool is_need_close() const {
		return _fd >= 0 && _nc;
	}

	void close() {
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

	static ptrdiff_t _read(int _fd, bytes_ref p) {
		return static_cast<ptrdiff_t>(::read(_fd, p.data(), p.size()));
	}

	static ptrdiff_t _write(int _fd, bytes_view p) {
		return static_cast<ptrdiff_t>(::write(_fd, p.data(), p.size()));
	}
};

}} // namespace rua::posix

#endif
