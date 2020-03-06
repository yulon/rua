#ifndef _RUA_STRING_LINE_HPP
#define _RUA_STRING_LINE_HPP

#include "string_view.hpp"

#include "../macros.hpp"
#include "../bytes.hpp"
#include "../io/util.hpp"

#include <cstdint>
#include <string>

namespace rua {

namespace eol {

RUA_MULTIDEF_VAR constexpr const char *lf = "\n";
RUA_MULTIDEF_VAR constexpr const char *crlf = "\r\n";
RUA_MULTIDEF_VAR constexpr const char *cr = "\r";

#ifdef _WIN32

RUA_MULTIDEF_VAR constexpr const char *sys_text = crlf;
RUA_MULTIDEF_VAR constexpr const char *sys_con = crlf;

#elif defined(RUA_DARWIN)

RUA_MULTIDEF_VAR constexpr const char *sys_text = cr;
RUA_MULTIDEF_VAR constexpr const char *sys_con = lf;

#else

RUA_MULTIDEF_VAR constexpr const char *sys_text = lf;
RUA_MULTIDEF_VAR constexpr const char *sys_con = lf;

#endif

} // namespace eol

inline bool is_eol(const char c) {
	return c == eol::lf[0] || c == eol::cr[0];
}

inline bool is_eol(string_view str) {
	for (auto &c : str) {
		if (!is_eol(c)) {
			return false;
		}
	}
	return true;
}

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
