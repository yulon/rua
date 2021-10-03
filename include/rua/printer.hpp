#ifndef _RUA_PRINTER_HPP
#define _RUA_PRINTER_HPP

#include "io.hpp"
#include "string.hpp"
#include "types/util.hpp"

namespace rua {

template <typename Writer>
class printer {
public:
	printer() {}

	printer(std::nullptr_t) : printer() {}

	explicit printer(Writer &w, const char *eol = eol::lf) :
		_w(&w), _eol(eol) {}

	~printer() {
		if (!_w) {
			return;
		}
		_w = nullptr;
	}

	operator bool() const {
		return _w && is_valid(*_w);
	}

	template <typename... Args>
	void print(Args &&...args) {
		if (!*this) {
			return;
		}
		str_join(_buf, {to_temp_string(args)...}, " ");
		_w->write_all(as_bytes(_buf));
		_buf.resize(0);
	}

	template <typename... Args>
	void println(Args &&...args) {
		print(std::forward<Args>(args)..., _eol);
	}

	void reset(Writer &w = nullptr) {
		_w = &w;
	}

	void reset(Writer &w, const char *eol) {
		_w = &w;
		_eol = eol;
	}

private:
	Writer *_w;
	const char *_eol;
	std::string _buf;
};

template <typename Writer>
printer<Writer> make_printer(Writer &w, const char *eol = eol::lf) {
	return printer<Writer>(w, eol);
}

} // namespace rua

#endif
