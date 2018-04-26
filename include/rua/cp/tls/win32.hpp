#ifndef _RUA_CP_TLS_WIN32_HPP
#define _RUA_CP_TLS_WIN32_HPP

#ifndef _WIN32
	#error rua::cp::tls: not supported this platform!
#endif

#include "../../gnc/any_word.hpp"
#include "../../gnc/any_ptr.hpp"

#include <windows.h>

#include <cassert>

namespace rua {
	namespace cp {
		class fls {
			public:
				static bool valid() {
					return _apis().alloc;
				}

				using native_handle_t = DWORD;

				constexpr fls(std::nullptr_t) : _ix(TLS_OUT_OF_INDEXES) {}

				fls(native_handle_t index) : _ix(index) {}

				fls() : _ix(_apis().alloc(nullptr)) {}

				~fls() {
					free();
				}

				fls(const fls &) = delete;

				fls &operator=(const fls &) = delete;

				fls(fls &&src) : _ix(src._ix) {
					if (src) {
						src._ix = TLS_OUT_OF_INDEXES;
					}
				}

				fls &operator=(fls &&src) {
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
					return _apis().set(_ix, value);
				}

				any_word get() const {
					return _apis().get(_ix);
				}

				bool alloc() {
					if (!free()) {
						return false;
					}
					_ix = _apis().alloc(nullptr);
					return *this;
				}

				bool free() {
					if (!*this) {
						return true;
					}
					if (!_apis().free(_ix)) {
						return false;
					}
					_ix = TLS_OUT_OF_INDEXES;
					return true;
				}

			private:
				native_handle_t _ix;

				struct _apis_t {
					DWORD (WINAPI *alloc)(PVOID);
					decltype(&TlsFree) free;
					decltype(&TlsGetValue) get;
					decltype(&TlsSetValue) set;
				};

				static _apis_t _load_apis() {
					_apis_t apis;
					auto kernel32 = GetModuleHandleW(L"kernel32.dll");
					apis.alloc = any_ptr(GetProcAddress(kernel32, "FlsAlloc"));
					if (!apis.alloc) {
						return apis;
					}
					apis.free = any_ptr(GetProcAddress(kernel32, "FlsFree"));
					apis.get = any_ptr(GetProcAddress(kernel32, "FlsGetValue"));
					apis.set = any_ptr(GetProcAddress(kernel32, "FlsSetValue"));
					return apis;
				}

				static _apis_t &_apis() {
					static _apis_t apis = _load_apis();
					return apis;
				}
		};

		class tls {
			public:
				using native_handle_t = DWORD;

				constexpr tls(std::nullptr_t) : _ix(TLS_OUT_OF_INDEXES) {}

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
