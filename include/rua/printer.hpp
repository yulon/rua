#ifndef _RUA_PRINTER_HPP
#define _RUA_PRINTER_HPP

#include "io.hpp"
#include "macros.hpp"
#include "string.hpp"
#include "sync.hpp"

#include <array>
#include <utility>

namespace rua {

class printer {
public:
	constexpr printer() : _mtx(), _w(), _eol(eol::sys) {}

	constexpr printer(std::nullptr_t) : printer() {}

	explicit printer(writer_i w, const char *eol = eol::sys) :
		_mtx(w ? new mutex : nullptr),
		_w(std::move(w)),
		_eol(eol) {}

	~printer() {
		if (!_mtx) {
			return;
		}
		{
			auto lg = make_lock_guard(*_mtx);
			if (_w) {
				_w.reset();
			}
		}
		_mtx.reset();
	}

	printer(printer &&src) {
		if (!src._mtx) {
			return;
		}
		auto lg = make_lock_guard(*src._mtx);
		_mtx = std::move(src._mtx);
		_w = std::move(src._w);
		_eol = src._eol;
	}

	RUA_OVERLOAD_ASSIGNMENT_R(printer)

	template <typename... Args>
	void print(Args &&... args) {
		if (!_mtx) {
			return;
		}
		auto lg = make_lock_guard(*_mtx);
		_print(std::forward<Args>(args)...);
	}

	template <typename... Args>
	void println(Args &&... args) {
		if (!_mtx) {
			return;
		}
		auto lg = make_lock_guard(*_mtx);
		_print(std::forward<Args>(args)...);
		_w->write_all(_eol);
	}

private:
	std::unique_ptr<mutex> _mtx;
	writer_i _w;
	const char *_eol;

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
