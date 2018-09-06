#ifndef _RUA_FILE_C_HPP
#define _RUA_FILE_C_HPP

#include "../io.hpp"

#include <cstddef>
#include <cstdio>

namespace rua { namespace c {
	class file : public io::read_writer {
		public:
			using native_handle_t = FILE *;

			file(native_handle_t f = nullptr) : _f(f) {}

			virtual ~file() = default;

			native_handle_t native_handle() const {
				return _f;
			}

			virtual size_t read(bin_ref p) {
				return static_cast<size_t>(fread(p.base(), 1, p.size(), native_handle()));
			}

			virtual size_t write(bin_view p) {
				return static_cast<size_t>(fwrite(p.base(), 1, p.size(), native_handle()));
			}

		private:
			native_handle_t _f;
	};
}}

#endif
