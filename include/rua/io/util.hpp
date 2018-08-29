#ifndef _RUA_IO_UTIL_HPP
#define _RUA_IO_UTIL_HPP

#include "itfs.hpp"

#include <cstddef>

namespace rua {
	namespace io {
		inline intmax_t reader::read_full(bin_ref p) {
			size_t tsz = 0;
			while (tsz < p.size()) {
				auto sz = read(p(tsz));
				if (sz < 0) {
					return sz;
				}
				tsz += static_cast<size_t>(sz);
			}
			return static_cast<intmax_t>(p.size());
		}

		inline bin reader::read_all(size_t buf_sz) {
			bin buf(buf_sz);
			size_t tsz = 0;
			for (;;) {
				auto sz = read(buf(tsz));
				if (sz < 0) {
					break;
				}
				tsz += static_cast<size_t>(sz);
				if (buf.size() - tsz < buf_sz / 2) {
					buf.resize(buf.size() + buf_sz);
				}
			}
			buf.resize(tsz);
			return buf;
		}

		inline bool writer::write_all(bin_view p) {
			size_t tsz = 0;
			while (tsz < p.size()) {
				auto sz = write(p(tsz));
				if (sz < 0) {
					return false;
				}
				tsz += static_cast<size_t>(sz);
			}
			return true;
		}
	}
}

#endif
