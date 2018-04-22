#ifndef _RUA_CP_TLS_WIN32_HPP
#define _RUA_CP_TLS_WIN32_HPP

#ifndef _WIN32
	#error rua::cp::tls: not supported this platform!
#endif

#include "../../gnc/any_word.hpp"

#include <windows.h>

namespace rua {
	namespace cp {
		class tls {
			public:
				using native_handle_t = DWORD;

				tls(std::nullptr_t) : _ix(TLS_OUT_OF_INDEXES) {}

				tls(native_handle_t index) : _ix(index) {}

				tls() : _ix(TlsAlloc()) {}

				~tls() {
					free();
				}

				tls(const tls &) = delete;

				tls &operator=(const tls &) = delete;

				tls(tls &&src) : _ix(src._ix) {
					if (src) {
						src._ix = TLS_OUT_OF_INDEXES;
					}
				}

				tls &operator=(tls &&src) {
					free();
					if (src) {
						_ix = src._ix;
						src._ix = TLS_OUT_OF_INDEXES;
					}
					return *this;
				}

				native_handle_t native_handle() const {
					return _ix;
				}

				operator native_handle_t() const {
					return _ix;
				}

				operator bool() const {
					return _ix != TLS_OUT_OF_INDEXES;
				}

				bool set(any_word value) {
					return TlsSetValue(_ix, value);
				}

				any_word get() const {
					return TlsGetValue(_ix);
				}

				bool alloc() {
					if (!free()) {
						return false;
					}
					_ix = TlsAlloc();
					return *this;
				}

				bool free() {
					if (!*this) {
						return true;
					}
					if (!TlsFree(_ix)) {
						return false;
					}
					_ix = TLS_OUT_OF_INDEXES;
					return true;
				}

			private:
				native_handle_t _ix;
		};
	}
}

#endif
