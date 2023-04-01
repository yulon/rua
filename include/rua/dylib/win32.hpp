#ifndef _rua_dylib_win32_hpp
#define _rua_dylib_win32_hpp

#include "dyfn.h"

#include "../any_ptr.hpp"
#include "../string/codec/base/win32.hpp"
#include "../string/view.hpp"
#include "../util.hpp"

#include <windows.h>

namespace rua { namespace win32 {

class dylib {
public:
	using native_handle_t = HMODULE;

	explicit dylib(string_view name, bool need_unload = true) :
		dylib(LoadLibraryW(u2w(name).c_str()), need_unload) {}

	constexpr dylib(native_handle_t h = nullptr, bool need_unload = true) :
		$h(h), $need_unload(need_unload && h) {}

	static dylib from_loaded(string_view name) {
		return dylib(GetModuleHandleW(u2w(name).c_str()), false);
	}

	static dylib from_loaded_or_load(string_view name) {
		auto dl = from_loaded(name);
		if (!dl) {
			dl = dylib(name);
		}
		return dl;
	}

	~dylib() {
		unload();
	}

	dylib(dylib &&src) : $h(src.$h), $need_unload(src.$need_unload) {
		if (!src.$h) {
			return;
		}
		src.$h = nullptr;
	}

	RUA_OVERLOAD_ASSIGNMENT(dylib)

	explicit operator bool() const {
		return $h;
	}

	native_handle_t native_handle() const {
		return $h;
	}

	bool has_ownership() const {
		return $h && $need_unload;
	}

	any_ptr find(string_view name) const {
		return $h ? GetProcAddress($h, name.data()) : nullptr;
	}

	any_ptr operator[](string_view name) const {
		return find(name);
	}

	void unload() {
		if (!$h) {
			return;
		}
		if ($need_unload) {
			FreeLibrary($h);
		}
		$h = nullptr;
	}

private:
	HMODULE $h;
	bool $need_unload;
};

// TODO: unique_dylib

}} // namespace rua::win32

#endif
