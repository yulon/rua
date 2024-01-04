#ifndef _rua_printer_hpp
#define _rua_printer_hpp

#include "io.hpp"
#include "string/join.hpp"
#include "util.hpp"

namespace rua {

template <typename... Args>
inline std::string sprint(Args &&...args) {
	return join({to_temp_string(args)...}, ' ');
}

class printer {
public:
	constexpr printer(std::nullptr_t = nullptr) : $w(nullptr), $eol(eol::lf) {}

	explicit printer(stream_i w, string_view eol = eol::lf) :
		$w(std::move(w)), $eol(eol) {}

	~printer() {
		if (!$w) {
			return;
		}
		$w = nullptr;
	}

	operator bool() const {
		return !!$w;
	}

	template <typename... Args>
	future<size_t> print(Args &&...args) {
		return $w->write(as_bytes(sprint(std::forward<Args>(args)...)));
	}

	template <typename... Args>
	future<size_t> println(Args &&...args) {
		return print(std::forward<Args>(args)..., $eol);
	}

	stream_i get() {
		return $w;
	}

	void reset(stream_i w = nullptr) {
		$w = std::move(w);
	}

	void reset(stream_i w, const char *eol) {
		$w = std::move(w);
		$eol = eol;
	}

private:
	stream_i $w;
	string_view $eol;
};

} // namespace rua

#endif
