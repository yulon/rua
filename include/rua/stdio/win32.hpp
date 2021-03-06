#ifndef _RUA_STDIO_WIN32_HPP
#define _RUA_STDIO_WIN32_HPP

#include "../io/sys_stream/win32.hpp"
#include "../macros.hpp"
#include "../string/char_enc/stream/win32.hpp"
#include "../types/util.hpp"

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
class _basic_stdio_stream : public read_write_closer {
public:
	virtual ptrdiff_t read(bytes_ref p) {
		return sys_stream(GetStdHandle(Id), false).read(p);
	}

	virtual ptrdiff_t write(bytes_view p) {
		return sys_stream(GetStdHandle(Id), false).write(p);
	}

	operator sys_stream() const {
		return sys_stream(GetStdHandle(Id), false);
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
		operator=(nullptr);
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

inline u8_to_loc_writer &sout() {
	static u8_to_loc_writer s(out());
	return s;
}

inline u8_to_loc_writer &serr() {
	static u8_to_loc_writer s(err());
	return s;
}

inline loc_to_u8_reader &sin() {
	static loc_to_u8_reader s(in());
	return s;
}

} // namespace _stdio

using namespace _stdio;

}} // namespace rua::win32

#endif
