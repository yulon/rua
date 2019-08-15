#ifndef _RUA_IO_UTIL_HPP
#define _RUA_IO_UTIL_HPP

#include "../sync/chan.hpp"
#include "../thread.hpp"
#include "abstract.hpp"

#include <atomic>
#include <cstddef>
#include <initializer_list>
#include <vector>

namespace rua {

inline size_t reader::read_full(bytes_ref p) {
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

inline bytes reader::read_all(size_t buf_grain_sz) {
	bytes buf(buf_grain_sz);
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

inline bool writer::write_all(bytes_view p) {
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

inline bool writer::copy(const reader_i &r, bytes_ref buf) {
	bytes inner_buf;
	if (!buf) {
		inner_buf.reset(1024);
		buf = inner_buf;
	}
	for (;;) {
		auto sz = r->read(buf);
		if (!sz) {
			return true;
		}
		if (!write_all(buf(0, sz))) {
			return false;
		}
	}
	return false;
}

class read_group : public reader {
public:
	read_group(size_t buf_sz = 1024) : _c(0), _buf_sz(buf_sz) {}

	void add(reader_i r) {
		++_c;
		thread([this, r]() {
			bytes buf(_buf_sz);
			for (;;) {
				auto sz = r->read(buf);
				if (!sz) {
					_ch << nullptr;
					return;
				}
				_ch << bytes(buf(0, sz));
			}
		});
	}

	virtual size_t read(bytes_ref p) {
		while (!_buf) {
			_ch >> _buf;
			if (!_buf) {
				if (--_c) {
					continue;
				}
				return 0;
			}
		}
		auto csz = p.copy_from(_buf);
		_buf = _buf(csz);
		return csz;
	}

private:
	std::atomic<size_t> _c, _buf_sz;
	chan<bytes> _ch;
	bytes _buf;
};

class write_group : public writer {
public:
	write_group() = default;

	write_group(std::initializer_list<writer_i> li) : _li(li) {}

	void add(writer_i w) {
		_li.emplace_back(std::move(w));
	}

	virtual size_t write(bytes_view p) {
		for (auto &w : _li) {
			w->write_all(p);
		}
		return p.size();
	}

private:
	std::vector<writer_i> _li;
};

} // namespace rua

#endif
