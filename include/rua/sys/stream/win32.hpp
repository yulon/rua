#ifndef _rua_sys_stream_win32_hpp
#define _rua_sys_stream_win32_hpp

#include "../../io/stream.hpp"
#include "../../thread/parallel.hpp"
#include "../../util.hpp"

#include <windows.h>

#include <cassert>
#include <cstdio>

namespace rua { namespace win32 {

class sys_stream : public stream {
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

	explicit operator bool() const override {
		return $h != INVALID_HANDLE_VALUE;
	}

	bool is_need_close() const {
		return $h && $nc;
	}

	future<> close() override {
		auto $ = self().as<sys_stream>();
		return stream::close() >> [$]() {
			if (!$) {
				return;
			}
			if ($->$nc) {
				CloseHandle($->$h);
			}
			$->$h = INVALID_HANDLE_VALUE;
		};
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

protected:
	expected<size_t> unbuf_sync_read(bytes_ref buf) override {
		assert(*this);

		DWORD n;
		if (ReadFile(
				$h, buf.data(), static_cast<DWORD>(buf.size()), &n, nullptr)) {
			return static_cast<size_t>(n);
		}
		return err_waiting_timeout;
	}

	expected<size_t> unbuf_sync_write(bytes_view data) override {
		assert(*this);

		DWORD n;
		if (WriteFile(
				$h,
				data.data(),
				static_cast<DWORD>(data.size()),
				&n,
				nullptr)) {
			return static_cast<size_t>(n);
		}
		printf("write err %d %p\n", GetLastError(), $h);
		return err_waiting_timeout;
	}

private:
	HANDLE $h;
	bool $nc;
};

}} // namespace rua::win32

#endif
