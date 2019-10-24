#ifndef _RUA_DYLIB_POSIX_HPP
#define _RUA_DYLIB_POSIX_HPP

#include "../generic_ptr.hpp"
#include "../macros.hpp"
#include "../string/string_view.hpp"

#include <dlfcn.h>

namespace rua { namespace posix {

class dylib {
public:
	using native_handle_t = void *;

	dylib(string_view name, bool shared_scope = true) {
		auto flags = RTLD_LAZY;
		if (shared_scope) {
			flags |= RTLD_GLOBAL;
		} else {
			flags |= RTLD_LOCAL;
		}
		_h = dlopen(name.data(), flags);
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

	generic_ptr get(string_view name) const {
		return dlsym(_h, name.data());
	}

	generic_ptr operator[](string_view name) const {
		return get(name);
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

}} // namespace rua::posix

#endif
