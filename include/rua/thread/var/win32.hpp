#ifndef _rua_thread_var_win32_hpp
#define _rua_thread_var_win32_hpp

#include "base.hpp"

#include "../../any_word.hpp"
#include "../../sys/listen/win32.hpp"
#include "../../util.hpp"

#include <windows.h>

#include <cassert>

namespace rua { namespace win32 {

class thread_word_var {
public:
	thread_word_var(void (*dtor)(any_word)) : $ix(TlsAlloc()), $dtor(dtor) {}

	~thread_word_var() {
		if (!is_storable()) {
			return;
		}
		if (!TlsFree($ix)) {
			return;
		}
		$ix = TLS_OUT_OF_INDEXES;
	}

	thread_word_var(thread_word_var &&src) : $ix(src.$ix) {
		if (src.is_storable()) {
			src.$ix = TLS_OUT_OF_INDEXES;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(thread_word_var)

	using native_handle_t = DWORD;

	native_handle_t native_handle() const {
		return $ix;
	}

	bool is_storable() const {
		return $ix != TLS_OUT_OF_INDEXES;
	}

	void set(any_word value) {
		$get(TlsGetValue($ix)) = value;
	}

	any_word get() const {
		auto val_ptr = TlsGetValue($ix);
		if (!val_ptr) {
			return 0;
		}
		return $get(val_ptr);
	}

	void reset() {
		auto val = get();
		if (!val) {
			return;
		}
		$dtor(val);
		set(0);
	}

private:
	DWORD $ix;
	void (*$dtor)(any_word);

	any_word &$get(LPVOID val_ptr) const {
		if (val_ptr) {
			return *reinterpret_cast<any_word *>(val_ptr);
		}
		auto p = new any_word;
		TlsSetValue($ix, p);
		auto h = OpenThread(SYNCHRONIZE, FALSE, GetCurrentThreadId());
		assert(h);
		auto dtor = $dtor;
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
