#ifndef _RUA_SYS_STREAM_WIN32_HPP
#define _RUA_SYS_STREAM_WIN32_HPP

#include "../../io/abstract.hpp"
#include "../../io/util.hpp"

#include "../../macros.hpp"
#include "../../sched/suspender.hpp"
#include "../../sched/wait/uni.hpp"

#include <windows.h>

#include <cassert>
#include <cstdio>

namespace rua { namespace win32 {

class sys_stream : public read_write_closer {
public:
	using native_handle_t = HANDLE;

	sys_stream(
		native_handle_t h = INVALID_HANDLE_VALUE, bool need_close = true) :
		_h(h ? h : INVALID_HANDLE_VALUE),
		_nc(h && h != INVALID_HANDLE_VALUE ? need_close : false) {}

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

	explicit operator bool() const {
		return _h != INVALID_HANDLE_VALUE;
	}

	virtual ptrdiff_t read(bytes_ref p) {
		assert(*this);

		auto spdr = this_suspender();
		if (!spdr->is_own_stack()) {
			auto buf = try_make_heap_buffer(p);
			if (buf) {
				auto sz = rua::wait(std::move(spdr), _read, _h, bytes_ref(buf));
				if (sz > 0) {
					p.copy_from(buf);
				}
				return sz;
			}
		}
		return rua::wait(std::move(spdr), _read, _h, p);
	}

	virtual ptrdiff_t write(bytes_view p) {
		assert(*this);

		auto spdr = this_suspender();
		if (!spdr->is_own_stack()) {
			auto data = try_make_heap_data(p);
			if (data) {
				return rua::wait(std::move(spdr), _write, _h, bytes_view(data));
			}
		}
		return rua::wait(std::move(spdr), _write, _h, p);
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

	static ptrdiff_t _read(HANDLE h, bytes_ref p) {
		DWORD rsz;
		return ReadFile(
				   h, p.data(), static_cast<DWORD>(p.size()), &rsz, nullptr)
				   ? static_cast<ptrdiff_t>(rsz)
				   : static_cast<ptrdiff_t>(0);
	}

	static ptrdiff_t _write(HANDLE h, bytes_view p) {
		DWORD wsz;
		return WriteFile(
				   h, p.data(), static_cast<DWORD>(p.size()), &wsz, nullptr)
				   ? static_cast<ptrdiff_t>(wsz)
				   : static_cast<ptrdiff_t>(0);
	}
};

}} // namespace rua::win32

#endif
