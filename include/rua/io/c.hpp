#ifndef _RUA_IO_C_HPP
#define _RUA_IO_C_HPP

#include "abstr.hpp"

#include <cstddef>
#include <cstdio>

namespace rua { namespace io { namespace c {
	class handle {
		public:
			using native_handle_t = FILE *;

			handle(native_handle_t c_file = nullptr) : _f(c_file) {}

			virtual ~handle() = default;

			native_handle_t &native_handle() {
				return _f;
			}

			native_handle_t native_handle() const {
				return _f;
			}

			operator bool() const {
				return _f;
			}

		protected:
			native_handle_t _f;
	};

	class reader : public virtual handle, public virtual io::reader {
		public:
			reader(native_handle_t c_file = nullptr) : handle(c_file) {}

			virtual size_t read(bin_ref p) {
				return static_cast<size_t>(fread(p.base(), 1, p.size(), _f));
			}
	};

	class writer : public virtual handle, public virtual io::writer {
		public:
			writer(native_handle_t c_file = nullptr) : handle(c_file) {}

			virtual size_t write(bin_view p) {
				return static_cast<size_t>(fwrite(p.base(), 1, p.size(), _f));
			}
	};

	class closer : public virtual handle, public virtual io::closer {
		public:
			closer(native_handle_t c_file = nullptr) : handle(c_file) {}

			virtual ~closer() {
				close();
			}

			closer(const closer &src) : handle(src._f) {}

			closer &operator=(const closer &src) {
				close();
				_f = src._f;
				return *this;
			}

			closer(closer &&src) : handle(src._f) {
				if (src) {
					src._f = nullptr;
				}
			}

			closer &operator=(closer &&src) {
				close();
				if (src) {
					_f = src._f;
					src._f = nullptr;
				}
				return *this;
			}

			virtual void close() {
				if (_f) {
					fclose(_f);
					_f = nullptr;
				}
			}

			void detach() {
				_f = nullptr;
			}
	};

	////////////////////////////////////////////////////////////////////////////

	class read_writer : public virtual handle, public reader, public writer {
		public:
			read_writer(native_handle_t c_file = nullptr) : handle(c_file) {}
	};

	class read_closer : public virtual handle, public reader, public closer {
		public:
			read_closer(native_handle_t c_file = nullptr) : handle(c_file) {}
	};

	class write_closer : public virtual handle, public writer, public closer {
		public:
			write_closer(native_handle_t c_file = nullptr) : handle(c_file) {}
	};

	class read_write_closer : public virtual handle, public read_writer, public closer {
		public:
			read_write_closer(native_handle_t c_file = nullptr) : handle(c_file) {}
	};
}}}

#endif
