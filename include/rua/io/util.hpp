#ifndef _RUA_IO_UTIL_HPP
#define _RUA_IO_UTIL_HPP

#include "itfs.hpp"

#include <cstddef>

namespace rua { namespace io {
	inline size_t reader::read_full(bin_ref p) {
		size_t tsz = 0;
		while (tsz < p.size()) {
			auto sz = read(p(tsz));
			if (!sz) {
				return tsz;
			}
			tsz += static_cast<size_t>(sz);
		}
		return tsz;
	}

	inline bin reader::read_all(size_t buf_sz) {
		bin buf(buf_sz);
		size_t tsz = 0;
		for (;;) {
			auto sz = read(buf(tsz));
			if (!sz) {
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
			if (!sz) {
				return false;
			}
			tsz += static_cast<size_t>(sz);
		}
		return true;
	}

	inline bool writer::copy(reader &r, size_t buf_sz) {
		bin buf(buf_sz);
		for (;;) {
			auto sz = r.read(buf);
			if (!sz) {
				return true;
			}
			if (!write_all(buf(0, sz))) {
				return false;
			}
		}
		return false;
	}
}}

#endif
