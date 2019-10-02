#ifndef _RUA_PRINTER_HPP
#define _RUA_PRINTER_HPP

#include "io.hpp"
#include "string.hpp"
#include "sync.hpp"

#include <array>
#include <atomic>
#include <initializer_list>
#include <string>
#include <utility>

namespace rua {

class printer {
public:
	printer() : _is_valid(false) {}

	printer(std::nullptr_t) : printer() {}

	explicit printer(writer_i w, const char *eol = eol::sys) :
		_w(std::move(w)),
		_eol(eol) {
		_is_valid = _w;
	}

	~printer() {
		if (!_is_valid) {
			return;
		}
		auto lg = make_lock_guard(_mtx);
		if (!_w) {
			return;
		}
		_w.reset();
	}

	template <typename... Args>
	void print(Args &&... args) {
		if (!_is_valid) {
			return;
		}
		auto lg = make_lock_guard(_mtx);
		if (!_w) {
			return;
		}
		str_join(
			_buf,
			std::initializer_list<string_view>{to_temp_string(args)...},
			" ",
			str_join_multi_line);
		_w->write_all(_buf);
		_buf.resize(0);
	}

	template <typename... Args>
	void println(Args &&... args) {
		print(std::forward<Args>(args)..., _eol);
	}

	void reset(writer_i w = nullptr, const char *eol = eol::sys) {
		auto lg = make_lock_guard(_mtx);
		_is_valid = w;
		_w = w;
		_eol = eol;
	}

private:
	mutex _mtx;
	writer_i _w;
	const char *_eol;
	std::atomic<bool> _is_valid;
	std::string _buf;
};

} // namespace rua

#endif
