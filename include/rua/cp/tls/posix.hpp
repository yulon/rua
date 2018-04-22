#ifndef _RUA_CP_TLS_POSIX_HPP
#define _RUA_CP_TLS_POSIX_HPP

#include "../../gnc/any_word.hpp"

#include <pthread.h>

namespace rua {
	namespace cp {
		class tls {
			public:
				using native_handle_t = pthread_key_t;

				tls(std::nullptr_t) : _invalid(true) {}

				tls(native_handle_t key) : _invalid(false), _key(index) {}

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
				bool _invalid;
				native_handle_t _key;
		};
	}
}

#endif
