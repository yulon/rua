#ifndef _RUA_POSIX_TLS_HPP
#define _RUA_POSIX_TLS_HPP

#include "../any_word.hpp"

#ifdef __has_include
	#if __has_include(<pthread.h>)
		#include <pthread.h>
	#else
		#error rua::posix::tls: not supported this platform!
	#endif
#else
	#include <pthread.h>
#endif

namespace rua {
namespace posix {

class tls {
public:
	using native_handle_t = pthread_key_t;

	constexpr tls(std::nullptr_t) : _key(0), _invalid(true) {}

	tls(native_handle_t key) : _key(key), _invalid(false) {}

	tls() {
		_invalid = pthread_key_create(&_key, nullptr);
	}

	~tls() {
		free();
	}

	tls(const tls &) = delete;

	tls &operator=(const tls &) = delete;

	tls(tls &&src) : _key(src._key), _invalid(src._invalid) {
		if (src) {
			src._invalid = true;
		}
	}

	tls &operator=(tls &&src) {
		free();
		if (src) {
			_key = src._key;
			_invalid = false;
			src._invalid = true;
		}
		return *this;
	}

	native_handle_t native_handle() const {
		return _key;
	}

	operator native_handle_t() const {
		return _key;
	}

	operator bool() const {
		return !_invalid;
	}

	bool set(any_word value) {
		return pthread_setspecific(_key, value) == 0;
	}

	any_word get() const {
		return pthread_getspecific(_key);
	}

	bool alloc() {
		if (!free()) {
			return false;
		}
		_invalid = pthread_key_create(&_key, nullptr);
		return *this;
	}

	bool free() {
		if (!*this) {
			return true;
		}
		if (pthread_key_delete(_key) != 0) {
			return false;
		}
		_invalid = true;
		return true;
	}

private:
	native_handle_t _key;
	bool _invalid;
};

}
}

#endif
