#ifndef _RUA_DYLIB_POSIX_HPP
#define _RUA_DYLIB_POSIX_HPP

#include "dyfn.h"

#include "../generic_ptr.hpp"
#include "../macros.hpp"
#include "../string/view.hpp"

#include <dlfcn.h>

namespace rua { namespace posix {

class dylib {
public:
	using native_handle_t = void *;

	dylib(string_view name) {
		_h = dlopen(name.data(), RTLD_LAZY | RTLD_GLOBAL);
		_need_unload = _h;
	}

	constexpr dylib(native_handle_t h = nullptr, bool need_unload = true) :
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
		return _h ? dlsym(_h, name.data()) : nullptr;
	}

	generic_ptr operator[](string_view name) const {
		return find(name);
	}

	void unload() {
		if (!_h) {
			return;
		}
		if (_need_unload) {
			dlclose(_h);
		}
		_h = nullptr;
	}

private:
	void *_h;
	bool _need_unload;
};

class unique_dylib {
public:
	using native_handle_t = void *;

	constexpr unique_dylib() : _h(nullptr) {}

	unique_dylib(string_view name) :
		_h(dlopen(name.data(), RTLD_LAZY | RTLD_LOCAL)) {}

	~unique_dylib() {
		unload();
	}

	unique_dylib(unique_dylib &&src) : _h(src._h) {
		if (!src._h) {
			return;
		}
		src._h = nullptr;
	}

	RUA_OVERLOAD_ASSIGNMENT_R(unique_dylib)

	explicit operator bool() const {
		return _h;
	}

	native_handle_t native_handle() const {
		return _h;
	}

	generic_ptr find(string_view name) const {
		return _h ? dlsym(_h, name.data()) : nullptr;
	}

	generic_ptr operator[](string_view name) const {
		return find(name);
	}

	void unload() {
		if (!_h) {
			return;
		}
		dlclose(_h);
		_h = nullptr;
	}

private:
	void *_h;
};

}} // namespace rua::posix

#endif
