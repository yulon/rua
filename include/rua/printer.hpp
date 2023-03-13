#ifndef _rua_printer_hpp
#define _rua_printer_hpp

#include "io.hpp"
#include "string/join.hpp"
#include "util.hpp"

namespace rua {

class printer {
public:
	constexpr printer(std::nullptr_t = nullptr) : _w(nullptr), _eol(eol::lf) {}

	explicit printer(stream_i w, string_view eol = eol::lf) :
		_w(std::move(w)), _eol(eol) {}

	~printer() {
		if (!_w) {
			return;
		}
		_w = nullptr;
	}

	operator bool() const {
		return _w;
	}

	template <typename... Args>
	void print(Args &&...args) {
		_w->write_all(as_bytes(join({to_temp_string(args)...}, ' ')));
	}

	template <typename... Args>
	void println(Args &&...args) {
		print(std::forward<Args>(args)..., _eol);
	}

	stream_i get() {
		return _w;
	}

	void reset(stream_i w = nullptr) {
		_w = std::move(w);
	}

	void reset(stream_i w, const char *eol) {
		_w = std::move(w);
		_eol = eol;
	}

private:
	stream_i _w;
	string_view _eol;
};

} // namespace rua

#endif
