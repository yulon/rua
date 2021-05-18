#ifndef _RUA_THREAD_VAR_WIN32_HPP
#define _RUA_THREAD_VAR_WIN32_HPP

#include "base.hpp"

#include "../../any_word.hpp"
#include "../../macros.hpp"
#include "../../sched/async/win32.hpp"
#include "../../types/util.hpp"

#include <windows.h>

#include <cassert>

namespace rua { namespace win32 {

class thread_word_var {
public:
	thread_word_var(void (*dtor)(any_word)) : _ix(TlsAlloc()), _dtor(dtor) {}

	~thread_word_var() {
		if (!is_storable()) {
			return;
		}
		if (!TlsFree(_ix)) {
			return;
		}
		_ix = TLS_OUT_OF_INDEXES;
	}

	thread_word_var(thread_word_var &&src) : _ix(src._ix) {
		if (src.is_storable()) {
			src._ix = TLS_OUT_OF_INDEXES;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT_R(thread_word_var)

	using native_handle_t = DWORD;

	native_handle_t native_handle() const {
		return _ix;
	}

	bool is_storable() const {
		return _ix != TLS_OUT_OF_INDEXES;
	}

	void set(any_word value) {
		_get(TlsGetValue(_ix)) = value;
	}

	any_word get() const {
		auto val_ptr = TlsGetValue(_ix);
		if (!val_ptr) {
			return 0;
		}
		return _get(val_ptr);
	}

	void reset() {
		auto val = get();
		if (!val) {
			return;
		}
		_dtor(val);
		set(0);
	}

private:
	DWORD _ix;
	void (*_dtor)(any_word);

	any_word &_get(LPVOID val_ptr) const {
		if (!val_ptr) {
			auto p = new any_word(0);
			TlsSetValue(_ix, p);
			auto h = OpenThread(SYNCHRONIZE, FALSE, GetCurrentThreadId());
			assert(h);
			auto dtor = _dtor;
			async(h, [=](bool) {
				dtor(*p);
				delete p;
				CloseHandle(h);
			});
			val_ptr = p;
		}
		return *reinterpret_cast<any_word *>(val_ptr);
	}
};

template <typename T>
using thread_var = basic_thread_var<T, thread_word_var>;

}} // namespace rua::win32

#endif
