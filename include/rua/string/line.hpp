#ifndef _RUA_STRING_LINE_HPP
#define _RUA_STRING_LINE_HPP

#include "../bin.hpp"
#include "../io/util.hpp"

#include <cstdint>
#include <string>

namespace rua {

static constexpr const char *lf = "\n";

static constexpr const char *crlf = "\r\n";

static constexpr const char *cr = "\r";

#ifdef _WIN32
static constexpr const char *eol = crlf;
#elif defined(RUA_DARWIN)
static constexpr const char *eol = cr;
#else
static constexpr const char *eol = lf;
#endif

inline bool is_eol(const std::string &str) {
	for (auto &chr : str) {
		if (chr != lf[0] && chr != cr[0]) {
			return false;
		}
	}
	return true;
}

class line_reader {
public:
	line_reader() : _r(nullptr), _1st_chr_is_cr(false) {}

	line_reader(reader_i r) : _r(std::move(r)), _1st_chr_is_cr(false) {}

	std::string read_line(size_t buf_sz = 256) {
		for (;;) {
			_buf.resize(buf_sz);
			auto sz = _r->read(_buf);
			if (!sz) {
				_r = nullptr;
				_1st_chr_is_cr = false;
				return std::move(_cache);
			}
			if (_1st_chr_is_cr) {
				_1st_chr_is_cr = false;
				if (_buf[0] == static_cast<uint8_t>('\n')) {
					_buf.slice_self(1);
				}
			}
			for (size_t i = 0; i < sz; ++i) {
				if (_buf[i] == static_cast<uint8_t>('\r')) {
					_cache += std::string(_buf.base().to<const char *>(), i);
					if (i == sz - 1) {
						_1st_chr_is_cr = true;
						return std::move(_cache);
					}

					auto end = i + 1;
					if (_buf[i + 1] == static_cast<uint8_t>('\n')) {
						_1st_chr_is_cr = true;
						end = i + 1;
					} else {
						end = i;
					}

					std::string r(std::move(_cache));
					_cache = std::string(
						(_buf.base() + end).to<const char *>(),
						_buf.size() - end);
					return r;

				} else if (_buf[i] == static_cast<uint8_t>('\n')) {
					_cache += std::string(_buf.base().to<const char *>(), i);
					return std::move(_cache);
				}
			}
			_cache += std::string(_buf.base(), sz);
		}
		return "";
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
	bin _buf;
	std::string _cache;
};

} // namespace rua

#endif
