#ifndef _RUA_IO_UTIL_HPP
#define _RUA_IO_UTIL_HPP

#include "abstr.hpp"
#include "../chan.hpp"

#include <cstddef>
#include <thread>
#include <atomic>

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

	inline bin reader::read_all(size_t buf_grain_sz) {
		bin buf(buf_grain_sz);
		size_t tsz = 0;
		for (;;) {
			auto sz = read(buf(tsz));
			if (!sz) {
				break;
			}
			tsz += static_cast<size_t>(sz);
			if (buf.size() - tsz < buf_grain_sz / 2) {
				buf.resize(buf.size() + buf_grain_sz);
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

	inline bool writer::copy(reader &r, bin_ref buf) {
		bin inner_buf;
		if (!buf) {
			inner_buf.reset(1024);
			buf = inner_buf;
		}
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

	class read_group : public virtual reader {
		public:
			read_group(size_t buf_sz = 1024) : _c(0), _buf_sz(buf_sz) {}

			void add(reader &r) {
				++_c;
				std::thread([this, &r]() {
					bin buf(_buf_sz);
					for (;;) {
						auto sz = r.read(buf);
						if (!sz) {
							_ch << nullptr;
							return;
						}
						_ch << bin(buf(0, sz));
					}
				}).detach();
			}

			virtual size_t read(bin_ref p) {
				while (!_buf) {
					_ch >> _buf;
					if (!_buf) {
						if (--_c) {
							continue;
						}
						return 0;
					}
				}
				auto csz = p.copy(_buf);
				_buf.slice_self(csz);
				return csz;
			}

		private:
			std::atomic<size_t> _c, _buf_sz;
			chan<bin> _ch;
			bin _buf;
	};
}}

#endif
