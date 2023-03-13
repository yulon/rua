#ifndef _rua_thread_var_win32_hpp
#define _rua_thread_var_win32_hpp

#include "base.hpp"

#include "../../generic_word.hpp"
#include "../../sys/listen/win32.hpp"
#include "../../util.hpp"

#include <windows.h>

#include <cassert>

namespace rua { namespace win32 {

class thread_word_var {
public:
	thread_word_var(void (*dtor)(generic_word)) :
		_ix(TlsAlloc()), _dtor(dtor) {}

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

	RUA_OVERLOAD_ASSIGNMENT(thread_word_var)

	using native_handle_t = DWORD;

	native_handle_t native_handle() const {
		return _ix;
	}

	bool is_storable() const {
		return _ix != TLS_OUT_OF_INDEXES;
	}

	void set(generic_word value) {
		_get(TlsGetValue(_ix)) = value;
	}

	generic_word get() const {
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
	void (*_dtor)(generic_word);

	generic_word &_get(LPVOID val_ptr) const {
		if (val_ptr) {
			return *reinterpret_cast<generic_word *>(val_ptr);
		}
		auto p = new generic_word;
		TlsSetValue(_ix, p);
		auto h = OpenThread(SYNCHRONIZE, FALSE, GetCurrentThreadId());
		assert(h);
		auto dtor = _dtor;
		sys_listen(h, [p, dtor, h]() {
			dtor(*p);
			delete p;
			CloseHandle(h);
		});
		return *p;
	}
};

template <typename T>
using thread_var = basic_thread_var<T, thread_word_var>;

}} // namespace rua::win32

#endif
