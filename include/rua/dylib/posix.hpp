#ifndef _RUA_DYLIB_POSIX_HPP
#define _RUA_DYLIB_POSIX_HPP

#include "../macros.hpp"

#include <dlfcn.h>

#include <string>

namespace rua { namespace posix {

class dylib {
public:
	using native_handle_t = void *;

	dylib(const std::string &name, bool shared_scope = true) {
		auto flags = RTLD_LAZY;
		if (shared_scope) {
			flags |= RTLD_GLOBAL;
		} else {
			flags |= RTLD_LOCAL;
		}
		_h = dlopen(name.c_str(), flags);
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
		return reinterpret_cast<FuncPtr>(dlsym(_h, name.c_str()));
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
