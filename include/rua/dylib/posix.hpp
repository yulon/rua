#ifndef _rua_dylib_posix_hpp
#define _rua_dylib_posix_hpp

#include "dyfn.h"

#include "../any_ptr.hpp"
#include "../string/view.hpp"
#include "../util.hpp"

#include <dlfcn.h>

namespace rua { namespace posix {

class dylib {
public:
	using native_handle_t = void *;

	dylib(string_view name, bool need_unload = true) :
		dylib(dlopen(name.data(), RTLD_GLOBAL), need_unload) {}

	constexpr dylib(native_handle_t h = nullptr, bool need_unload = true) :
		$h(h), $need_unload(need_unload && h) {}

	static dylib from_loaded(string_view name) {
		return dylib(dlopen(name.data(), RTLD_NOLOAD | RTLD_GLOBAL), false);
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
		return $h ? dlsym($h, name.data()) : nullptr;
	}

	any_ptr operator[](string_view name) const {
		return find(name);
	}

	void unload() {
		if (!$h) {
			return;
		}
		if ($need_unload) {
			dlclose($h);
		}
		$h = nullptr;
	}

private:
	void *$h;
	bool $need_unload;
};

class unique_dylib {
public:
	using native_handle_t = void *;

	constexpr unique_dylib() : $h(nullptr) {}

	unique_dylib(string_view name) : $h(dlopen(name.data(), RTLD_LOCAL)) {}

	~unique_dylib() {
		unload();
	}

	unique_dylib(unique_dylib &&src) : $h(src.$h) {
		if (!src.$h) {
			return;
		}
		src.$h = nullptr;
	}

	RUA_OVERLOAD_ASSIGNMENT(unique_dylib)

	explicit operator bool() const {
		return $h;
	}

	native_handle_t native_handle() const {
		return $h;
	}

	any_ptr find(string_view name) const {
		return $h ? dlsym($h, name.data()) : nullptr;
	}

	any_ptr operator[](string_view name) const {
		return find(name);
	}

	void unload() {
		if (!$h) {
			return;
		}
		dlclose($h);
		$h = nullptr;
	}

private:
	void *$h;
};

}} // namespace rua::posix

#endif
