#ifndef _RUA_SYS_STREAM_WIN32_HPP
#define _RUA_SYS_STREAM_WIN32_HPP

#include "../../io/util.hpp"
#include "../../macros.hpp"
#include "../../sched.hpp"

#include <windows.h>

#include <cassert>
#include <cstdio>

namespace rua { namespace win32 {

class sys_stream : public stream_base {
public:
	using native_handle_t = HANDLE;

	sys_stream(
		native_handle_t h = INVALID_HANDLE_VALUE, bool need_close = true) :
		_h(h ? h : INVALID_HANDLE_VALUE),
		_nc(h && h != INVALID_HANDLE_VALUE && need_close) {}

	sys_stream(const sys_stream &src) {
		if (!src) {
			_h = INVALID_HANDLE_VALUE;
			return;
		}
		if (!src._nc) {
			_h = src._h;
			_nc = false;
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

	virtual operator bool() const {
		return _h != INVALID_HANDLE_VALUE;
	}

	virtual ssize_t read(bytes_ref buf) {
		assert(*this);

		auto dzr = this_dozer();
		if (dzr->is_unowned_data(buf)) {
			bytes buf_s(buf.size());
			auto sz = await(std::move(dzr), _read, _h, bytes_ref(buf_s));
			if (sz > 0) {
				buf.copy(buf_s);
			}
			return sz;
		}
		return await(std::move(dzr), _read, _h, buf);
	}

	virtual ssize_t write(bytes_view data) {
		assert(*this);

		auto dzr = this_dozer();
		if (dzr->is_unowned_data(data)) {
			bytes data_s(data);
			return await(std::move(dzr), _write, _h, bytes_view(data_s));
		}
		return await(std::move(dzr), _write, _h, data);
	}

	bool is_need_close() const {
		return _h && _nc;
	}

	virtual void close() {
		if (!*this) {
			return;
		}
		if (_nc) {
			CloseHandle(_h);
		}
		_h = INVALID_HANDLE_VALUE;
	}

	void detach() {
		_nc = false;
	}

	sys_stream dup(bool inherit = false) const {
		if (!*this) {
			return {};
		}
		HANDLE h_cp;
		DuplicateHandle(
			GetCurrentProcess(),
			_h,
			GetCurrentProcess(),
			&h_cp,
			0,
			inherit,
			DUPLICATE_SAME_ACCESS);
		assert(h_cp);
		return h_cp;
	}

private:
	HANDLE _h;
	bool _nc;

	static ssize_t _read(HANDLE h, bytes_ref p) {
		DWORD rsz;
		return ReadFile(
				   h, p.data(), static_cast<DWORD>(p.size()), &rsz, nullptr)
				   ? static_cast<ssize_t>(rsz)
				   : static_cast<ssize_t>(0);
	}

	static ssize_t _write(HANDLE h, bytes_view p) {
		DWORD wsz;
		return WriteFile(
				   h, p.data(), static_cast<DWORD>(p.size()), &wsz, nullptr)
				   ? static_cast<ssize_t>(wsz)
				   : static_cast<ssize_t>(0);
	}
};

}} // namespace rua::win32

#endif
