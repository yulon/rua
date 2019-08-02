#ifndef _RUA_DYLIB_WIN32_HPP
#define _RUA_DYLIB_WIN32_HPP

#include "../macros.hpp"
#include "../string/encoding/base/win32.hpp"

#include <windows.h>

namespace rua { namespace win32 {

class dylib {
public:
	using native_handle_t = HMODULE;

	dylib(const std::string &name, bool always_load = true) {
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

	constexpr dylib(native_handle_t h = nullptr, bool need_unload = false) :
		_h(h),
		_need_unload(need_unload) {}

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

	bool operator!() const {
		return !_h;
	}

	native_handle_t native_handle() const {
		return _h;
	}

	bool has_ownership() const {
		return _h && _need_unload;
	}

	template <typename FuncPtr>
	FuncPtr get(const std::string &name) const {
		return reinterpret_cast<FuncPtr>(GetProcAddress(_h, name.c_str()));
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

}} // namespace rua::win32

#endif
