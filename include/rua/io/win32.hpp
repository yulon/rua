#ifndef _RUA_IO_WIN32_HPP
#define _RUA_IO_WIN32_HPP

#ifndef _WIN32
	#error rua::io::win32: not supported this platform!
#endif

#include "abstr.hpp"

#include <windows.h>

#include <cassert>

namespace rua { namespace io { namespace win32 {
	class handle {
		public:
			using native_handle_t = HANDLE;

			handle(native_handle_t h = nullptr) : _h(h) {}

			virtual ~handle() {
				if (_h) {
					_h = nullptr;
				}
			}

			handle(const handle &src) : _h(src._h) {}

			handle &operator=(const handle &src) {
				this->~handle();
				_h = src._h;
				return *this;
			}

			handle(handle &&src) : _h(src._h) {
				if (src) {
					src._h = nullptr;
				}
			}

			handle &operator=(handle &&src) {
				this->~handle();
				if (src) {
					_h = src._h;
					src._h = nullptr;
				}
				return *this;
			}

			native_handle_t &native_handle() {
				return _h;
			}

			native_handle_t native_handle() const {
				return _h;
			}

			operator bool() const {
				return _h;
			}

		protected:
			native_handle_t _h;
	};

	class reader : public virtual handle, public virtual io::reader {
		public:
			reader(native_handle_t h = nullptr) : handle(h) {}

			virtual size_t read(bin_ref p) {
				DWORD rsz;
				return ReadFile(_h, p.base(), p.size(), &rsz, nullptr) ? static_cast<size_t>(rsz) : static_cast<size_t>(0);
			}
	};

	class writer : public virtual handle, public virtual io::writer {
		public:
			writer(native_handle_t h = nullptr) : handle(h) {}

			virtual size_t write(bin_view p) {
				DWORD wsz;
				return WriteFile(_h, p.base(), p.size(), &wsz, nullptr) ? static_cast<size_t>(wsz) : static_cast<size_t>(0);
			}
	};

	class closer : public virtual handle, public virtual io::closer {
		public:
			closer(native_handle_t h = nullptr) : handle(h) {}

			virtual ~closer() {
				if (_h) {
					CloseHandle(_h);
				}
			}

			virtual void close() {
				this->~closer();
			}

			void detach() {
				if (_h) {
					_h = nullptr;
				}
			}
	};

	////////////////////////////////////////////////////////////////////////////

	class read_writer : public virtual handle, public reader, public writer {
		public:
			read_writer(native_handle_t h = nullptr) : handle(h) {}
	};

	class read_closer : public virtual handle, public reader, public closer {
		public:
			read_closer(native_handle_t h = nullptr) : handle(h) {}
	};

	class write_closer : public virtual handle, public writer, public closer {
		public:
			write_closer(native_handle_t h = nullptr) : handle(h) {}
	};

	class read_write_closer : public virtual handle, public read_writer, public closer {
		public:
			read_write_closer(native_handle_t h = nullptr) : handle(h) {}
	};
}}}

#endif
