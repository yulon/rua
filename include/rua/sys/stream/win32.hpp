#ifndef _rua_sys_stream_win32_hpp
#define _rua_sys_stream_win32_hpp

#include "../../io/util.hpp"
#include "../../util.hpp"

#include <windows.h>

#include <cassert>
#include <cstdio>

namespace rua { namespace win32 {

class sys_stream : public stream_base {
public:
	using native_handle_t = HANDLE;

	sys_stream(
		native_handle_t h = INVALID_HANDLE_VALUE, bool need_close = true) :
		$h(h ? h : INVALID_HANDLE_VALUE),
		$nc(h && h != INVALID_HANDLE_VALUE && need_close) {}

	sys_stream(const sys_stream &src) {
		if (!src) {
			$h = INVALID_HANDLE_VALUE;
			return;
		}
		if (!src.$nc) {
			$h = src.$h;
			$nc = false;
			return;
		}
		DuplicateHandle(
			GetCurrentProcess(),
			src.$h,
			GetCurrentProcess(),
			&$h,
			0,
			FALSE,
			DUPLICATE_SAME_ACCESS);
		assert($h);
		$nc = true;
	}

	sys_stream(sys_stream &&src) : sys_stream(src.$h, src.$nc) {
		src.detach();
	}

	RUA_OVERLOAD_ASSIGNMENT(sys_stream)

	virtual ~sys_stream() {
		close();
	}

	native_handle_t &native_handle() {
		return $h;
	}

	native_handle_t native_handle() const {
		return $h;
	}

	virtual operator bool() const {
		return $h != INVALID_HANDLE_VALUE;
	}

	virtual ssize_t read(bytes_ref buf) {
		assert(*this);

		return $read($h, buf);
	}

	virtual ssize_t write(bytes_view data) {
		assert(*this);

		return $write($h, data);
	}

	bool is_need_close() const {
		return $h && $nc;
	}

	virtual void close() {
		if (!*this) {
			return;
		}
		if ($nc) {
			CloseHandle($h);
		}
		$h = INVALID_HANDLE_VALUE;
	}

	void detach() {
		$nc = false;
	}

	sys_stream dup(bool inherit = false) const {
		if (!*this) {
			return {};
		}
		HANDLE h_cp;
		DuplicateHandle(
			GetCurrentProcess(),
			$h,
			GetCurrentProcess(),
			&h_cp,
			0,
			inherit,
			DUPLICATE_SAME_ACCESS);
		assert(h_cp);
		return h_cp;
	}

private:
	HANDLE $h;
	bool $nc;

	static ssize_t $read(HANDLE h, bytes_ref p) {
		DWORD rsz;
		return ReadFile(
				   h, p.data(), static_cast<DWORD>(p.size()), &rsz, nullptr)
				   ? static_cast<ssize_t>(rsz)
				   : static_cast<ssize_t>(0);
	}

	static ssize_t $write(HANDLE h, bytes_view p) {
		DWORD wsz;
		return WriteFile(
				   h, p.data(), static_cast<DWORD>(p.size()), &wsz, nullptr)
				   ? static_cast<ssize_t>(wsz)
				   : static_cast<ssize_t>(0);
	}
};

}} // namespace rua::win32

#endif
