#ifndef _RUA_STRING_LINE_STREAM_HPP
#define _RUA_STRING_LINE_STREAM_HPP

#include "base.hpp"

#include "../../bytes.hpp"
#include "../../io.hpp"
#include "../../types/util.hpp"

#include <string>

namespace rua {

class line_reader {
public:
	constexpr line_reader() : _r(nullptr), _1st_chr_is_cr(false) {}

	line_reader(reader_i r) : _r(std::move(r)), _1st_chr_is_cr(false) {}

	std::string read_line(size_t buf_sz = 256) {
		std::string r;
		for (;;) {
			_buf.resize(buf_sz);
			auto sz = _r->read(_buf);
			if (sz <= 0) {
				_r = nullptr;
				_1st_chr_is_cr = false;
				r = static_cast<std::string>(_cache);
				_cache.resize(0);
				return r;
			}
			if (_1st_chr_is_cr) {
				_1st_chr_is_cr = false;
				if (_buf[0] == static_cast<byte>('\n')) {
					_buf = _buf(1);
				}
			}
			for (ptrdiff_t i = 0; i < sz; ++i) {
				if (_buf[i] == static_cast<byte>('\r')) {
					_cache += _buf(0, i);
					if (i == sz - 1) {
						_1st_chr_is_cr = true;
						r = static_cast<std::string>(_cache);
						_cache.resize(0);
						return r;
					}

					auto end = i + 1;
					if (_buf[i + 1] == static_cast<byte>('\n')) {
						_1st_chr_is_cr = true;
						end = i + 1;
					} else {
						end = i;
					}

					r = static_cast<std::string>(_cache);
					_cache = _buf(end);
					return r;

				} else if (_buf[i] == static_cast<byte>('\n')) {
					_cache += _buf(0, i);
					r = static_cast<std::string>(_cache);
					_cache.resize(0);
					return r;
				}
			}
			if (_buf) {
				_cache += _buf(0, sz);
			}
		}
		return r;
	}

	operator bool() const {
		return _r;
	}

	void reset(reader_i r) {
		_r = std::move(r);
		_1st_chr_is_cr = false;
		_cache = "";
	}

	void reset() {
		if (!_r) {
			return;
		}
		_r = nullptr;
		_1st_chr_is_cr = false;
		_cache = "";
	}

private:
	reader_i _r;
	bool _1st_chr_is_cr;
	bytes _buf, _cache;
};

} // namespace rua

#endif
