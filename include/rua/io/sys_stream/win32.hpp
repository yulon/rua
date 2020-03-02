#ifndef _RUA_IO_SYS_STREAM_WIN32_HPP
#define _RUA_IO_SYS_STREAM_WIN32_HPP

#include "../../macros.hpp"
#include "../abstract.hpp"

#include <windows.h>

#include <cassert>

namespace rua { namespace win32 {

class sys_stream : public read_write_closer {
public:
	using native_handle_t = HANDLE;

	constexpr sys_stream(native_handle_t h = nullptr, bool need_close = true) :
		_h(h),
		_nc(need_close) {}

	sys_stream(const sys_stream &src) {
		if (!src) {
			_h = nullptr;
			return;
		}
		DuplicateHandle(
			GetCurrentProcess(),
			src._h,
			GetCurrentProcess(),
			&_h,
			0,
			FALSE,
			DUPLICATE_SAME_ACCESS);
		assert(_h);
		_nc = true;
	}

	sys_stream(sys_stream &&src) : sys_stream(src._h, src._nc) {
		src.detach();
	}

	RUA_OVERLOAD_ASSIGNMENT(sys_stream)

	virtual ~sys_stream() {
		close();
	}

	native_handle_t &native_handle() {
		return _h;
	}

	native_handle_t native_handle() const {
		return _h;
	}

	explicit operator bool() const {
		return _h;
	}

	virtual ptrdiff_t read(bytes_ref p) {
		DWORD rsz;
		return ReadFile(
				   _h, p.data(), static_cast<DWORD>(p.size()), &rsz, nullptr)
				   ? static_cast<ptrdiff_t>(rsz)
				   : static_cast<ptrdiff_t>(0);
	}

	virtual ptrdiff_t write(bytes_view p) {
		DWORD wsz;
		return WriteFile(
				   _h, p.data(), static_cast<DWORD>(p.size()), &wsz, nullptr)
				   ? static_cast<ptrdiff_t>(wsz)
				   : static_cast<ptrdiff_t>(0);
	}

	virtual void close() {
		if (!_h) {
			return;
		}
		if (_nc) {
			CloseHandle(_h);
		}
		_h = nullptr;
	}

	void detach() {
		_nc = false;
	}

private:
	native_handle_t _h;
	bool _nc;
};

}} // namespace rua::win32

#endif
