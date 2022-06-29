#ifndef _RUA_DYLIB_WIN32_HPP
#define _RUA_DYLIB_WIN32_HPP

#include "dyfn.h"

#include "../generic_ptr.hpp"
#include "../string/char_codec/base/win32.hpp"
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
		_h(h), _need_unload(need_unload && h) {}

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

	dylib(dylib &&src) : _h(src._h), _need_unload(src._need_unload) {
		if (!src._h) {
			return;
		}
		src._h = nullptr;
	}

	RUA_OVERLOAD_ASSIGNMENT_R(dylib)

	explicit operator bool() const {
		return _h;
	}

	native_handle_t native_handle() const {
		return _h;
	}

	bool has_ownership() const {
		return _h && _need_unload;
	}

	generic_ptr find(string_view name) const {
		return _h ? GetProcAddress(_h, name.data()) : nullptr;
	}

	generic_ptr operator[](string_view name) const {
		return find(name);
	}

	void unload() {
		if (!_h) {
			return;
		}
		if (_need_unload) {
			FreeLibrary(_h);
		}
		_h = nullptr;
	}

private:
	HMODULE _h;
	bool _need_unload;
};

// TODO: unique_dylib

}} // namespace rua::win32

#endif
