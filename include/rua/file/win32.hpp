#ifndef _RUA_FILE_WIN32_HPP
#define _RUA_FILE_WIN32_HPP

#ifndef _WIN32
	#error rua::io::win32: not supported this platform!
#endif

#include "../io.hpp"

#include <windows.h>

#include <cassert>

namespace rua { namespace win32 {
	class file : public io::read_writer {
		public:
			using native_handle_t = HANDLE;

			file(native_handle_t f = nullptr) : _f(f) {}

			virtual ~file() = default;

			native_handle_t native_handle() const {
				return _f;
			}

			virtual size_t read(bin_ref p) {
				DWORD rsz;
				return ReadFile(_f, p.base(), p.size(), &rsz, nullptr) ? static_cast<size_t>(0) : static_cast<size_t>(rsz);
			}

			virtual size_t write(bin_view p) {
				DWORD wsz;
				return WriteFile(_f, p.base(), p.size(), &wsz, nullptr) ? static_cast<size_t>(0) : static_cast<size_t>(wsz);
			}

		private:
			native_handle_t _f;
	};
}}

#endif
