#ifndef _RUA_PRINTER_HPP
#define _RUA_PRINTER_HPP

#include "io.hpp"
#include "string.hpp"
#include "sync.hpp"

#include <array>
#include <atomic>
#include <utility>

namespace rua {

class printer {
public:
	constexpr printer() : _mtx(), _w(), _eol(eol::sys), _is_valid(false) {}

	constexpr printer(std::nullptr_t) : printer() {}

	explicit printer(writer_i w, const char *eol = eol::sys) :
		_mtx(),
		_w(std::move(w)),
		_eol(eol),
		_is_valid(w) {}

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
		_print(std::forward<Args>(args)...);
	}

	template <typename... Args>
	void println(Args &&... args) {
		if (!_is_valid) {
			return;
		}
		auto lg = make_lock_guard(_mtx);
		if (!_w) {
			return;
		}
		_print(std::forward<Args>(args)...);
		_w->write_all(_eol);
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

	template <typename... Args>
	void _print(Args &&... args) {
		std::array<std::string, sizeof...(args)> strs{to_string(args)...};
		for (auto &str : strs) {
			_w->write_all(str);
			if (&str != &strs.back()) {
				_w->write_all(" ");
			}
		}
	}
};

} // namespace rua

#endif
