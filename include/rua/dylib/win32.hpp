#ifndef _RUA_DYLIB_WIN32_HPP
#define _RUA_DYLIB_WIN32_HPP

#include "dyfn.h"

#include "../generic_ptr.hpp"
#include "../macros.hpp"
#include "../string/encoding/base/win32.hpp"
#include "../string/string_view.hpp"

#include <windows.h>

namespace rua { namespace win32 {

class dylib {
public:
	using native_handle_t = HMODULE;

	explicit dylib(string_view name, bool always_load = true) {
		_load(name, always_load);
	}

	constexpr dylib(native_handle_t h = nullptr, bool need_unload = false) :
		_h(h), _need_unload(need_unload) {}

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
		return GetProcAddress(_h, name.data());
	}

	generic_ptr operator[](string_view name) const {
		return find(name);
	}

	bool load(string_view name, bool always_load = true) {
		unload();
		_load(name, always_load);
		return _h;
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

	void _load(string_view name, bool always_load) {
		auto wname = u8_to_w(name);
		if (!always_load) {
			_h = GetModuleHandleW(wname.c_str());
			if (_h) {
				_need_unload = false;
				return;
			}
		}
		_h = LoadLibraryW(wname.c_str());
		_need_unload = _h;
	}
};

}} // namespace rua::win32

#endif
