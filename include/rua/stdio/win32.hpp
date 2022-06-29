#ifndef _RUA_STDIO_WIN32_HPP
#define _RUA_STDIO_WIN32_HPP

#include "../string/codec/stream/win32.hpp"
#include "../sys/stream/win32.hpp"
#include "../util.hpp"

#include "windows.h"

#include <atomic>
#include <cassert>

namespace rua { namespace win32 {

namespace _stdio {

template <DWORD Id>
class _basic_stdio_stream;

using stdout_stream = _basic_stdio_stream<STD_OUTPUT_HANDLE>;
using stderr_stream = _basic_stdio_stream<STD_ERROR_HANDLE>;
using stdin_stream = _basic_stdio_stream<STD_INPUT_HANDLE>;

template <DWORD Id>
class _basic_stdio_stream : public stream_base {
public:
	using native_handle_t = HANDLE;

	virtual ssize_t read(bytes_ref p) {
		return sys_stream(GetStdHandle(Id), false).read(p);
	}

	virtual ssize_t write(bytes_view p) {
		return sys_stream(GetStdHandle(Id), false).write(p);
	}

	native_handle_t native_handle() const {
		return GetStdHandle(Id);
	}

	operator sys_stream() const {
		return sys_stream(GetStdHandle(Id), false);
	}

	virtual operator bool() const {
		return GetStdHandle(Id);
	}

	_basic_stdio_stream &operator=(sys_stream s) {
		auto h = s.native_handle();
		if (h) {
			if (!s.is_need_close()) {
				s = s.dup();
				h = s.native_handle();
			}
			s.detach();
		}

		SetStdHandle(Id, h);

		static std::atomic<HANDLE> h_cache;
		auto old_h = h_cache.exchange(h);
		if (!old_h) {
			return *this;
		}
		CloseHandle(old_h);
		return *this;
	}

	virtual void close() {
		operator=(INVALID_HANDLE_VALUE);
	}

private:
	_basic_stdio_stream() = default;
	_basic_stdio_stream(const _basic_stdio_stream &) = delete;
	_basic_stdio_stream(_basic_stdio_stream &&) = delete;

	friend stdout_stream &out();
	friend stderr_stream &err();
	friend stdin_stream &in();
};

inline stdout_stream &out() {
	static stdout_stream s;
	return s;
}

inline stderr_stream &err() {
	static stderr_stream s;
	return s;
}

inline stdin_stream &in() {
	static stdin_stream s;
	return s;
}

inline u2l_writer &sout() {
	static u2l_writer s(out());
	return s;
}

inline u2l_writer &serr() {
	static u2l_writer s(err());
	return s;
}

inline l2u_reader &sin() {
	static l2u_reader s(in());
	return s;
}

} // namespace _stdio

using namespace _stdio;

}} // namespace rua::win32

#endif
