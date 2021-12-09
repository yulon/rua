#ifndef _RUA_STRING_LINE_READER_HPP
#define _RUA_STRING_LINE_READER_HPP

#include "char_set.hpp"

#include "../bytes.hpp"
#include "../io.hpp"
#include "../optional.hpp"
#include "../types/util.hpp"

#include <list>
#include <string>

namespace rua {

template <typename Reader>
class line_reader {
public:
	line_reader() : _r(nullptr), _buf(), _data(), _prev_b{0} {}

	line_reader(Reader &r) : _r(&r), _prev_b{0} {}

	line_reader(line_reader &&src) :
		_r(src._r),
		_buf(std::move(src._buf)),
		_data(src._data),
		_prev_b(src._prev_b) {
		if (src) {
			src._r = nullptr;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT_R(line_reader)

	optional<std::string> read_line(size_t buf_sz = 1024) {
		if (_buf.size() != buf_sz) {
			_buf.resize(buf_sz);
		}
		std::string ln;
		for (;;) {
			for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(_data.size());
				 ++i) {
				if (_data[i] == '\n') {
					if (_prev_b != '\r') {
						ln += as_string(_data(0, i));
						_prev_b = _data[i];
						_data = _data(i + 1);
						return ln;
					}
					_prev_b = _data[i];
					_data = _data(i + 1);
					i = -1;
				} else if (_data[i] == '\r') {
					ln += as_string(_data(0, i));
					_prev_b = _data[i];
					_data = _data(i + 1);
					return ln;
				} else {
					_prev_b = _data[i];
				}
			}
			if (_data) {
				ln += as_string(_data);
				_data.reset();
			}

			auto sz = _r->read(_buf);
			if (sz <= 0) {
				if (ln.length()) {
					return ln;
				}
				return nullopt;
			}
			_data = _buf(0, sz);
		}
		return nullopt;
	}

	std::list<std::string> read_lines(size_t buf_sz = 1024) {
		std::list<std::string> li;
		for (;;) {
			auto ln_opt = read_line(buf_sz);
			if (!ln_opt) {
				return li;
			}
			li.push_back(*ln_opt);
		}
		return li;
	}

	operator bool() const {
		return is_valid(_r);
	}

	void reset() {
		if (!_r) {
			return;
		}
		_r = nullptr;
	}

private:
	Reader *_r;
	bytes _buf;
	bytes_ref _data;
	uchar _prev_b;
};

template <typename Reader>
line_reader<Reader> make_line_reader(Reader &r) {
	return r;
}

} // namespace rua

#endif
