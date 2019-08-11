#ifndef _RUA_STRING_STRING_VIEW_HPP
#define _RUA_STRING_STRING_VIEW_HPP

#include "../macros.hpp"

#ifdef __cpp_lib_string_view

#include <string_view>

namespace rua {
template <typename CharT, typename Traits = std::char_traits<CharT> >
using basic_string_view = std::basic_string_view<CharT, Traits>;
}

#else

#include <cstdint>
#include <cstring>
#include <string>

namespace rua {

template <typename CharT, typename Traits = std::char_traits<CharT> >
class basic_string_view {
public:
	constexpr basic_string_view() : _s(nullptr), _c(0), _max_c(0) {}

	constexpr basic_string_view(std::nullptr_t) : basic_string_view() {}

	static constexpr auto _nullsz = static_cast<size_t>(-3);

	constexpr basic_string_view(const CharT *s, size_t count = _nullsz) :
		_s(s),
		_c(count),
		_max_c(count) {}

	basic_string_view(const std::basic_string<CharT, Traits> &s) :
		basic_string_view(s.c_str(), s.size()) {}

	operator std::basic_string<CharT, Traits>() const {
		return std::basic_string<CharT, Traits>(data(), size());
	}

	basic_string_view(const basic_string_view &src) :
		_s(src._s),
		_c(src._c),
		_max_c(src._max_c) {}

	RUA_OVERLOAD_ASSIGNMENT_L(basic_string_view)

	size_t size() {
		if (_max_c == _nullsz) {
			_c = _calc_len();
			_max_c = _c;
		}
		return _c;
	}

	size_t size() const {
		if (_max_c == _nullsz) {
			return _calc_len();
		}
		return _c;
	}

	size_t max_size() {
		if (_max_c == _nullsz) {
			_c = _calc_len();
			_max_c = _c;
		}
		return _max_c;
	}

	size_t max_size() const {
		if (_max_c == _nullsz) {
			return _calc_len();
		}
		return _c;
	}

	size_t length() {
		return size();
	}

	size_t length() const {
		return size();
	}

	constexpr const CharT *data() const {
		return _s;
	}

	constexpr bool empty() const {
		return !size();
	}

	const CharT &operator[](ptrdiff_t ix) const {
		return _s[ix];
	}

	const CharT *begin() const {
		return data();
	}

	const CharT *cbegin() const {
		return data();
	}

	const CharT *end() const {
		return begin() + size();
	}

	const CharT *cend() const {
		return cbegin() + size();
	}

	static constexpr size_t npos = static_cast<size_t>(-1);

	size_t find(basic_string_view v, size_t pos = 0) const {
		auto vsz = v.size();
		auto end_ix = size() - vsz + 1;
		for (size_t i = pos; i < end_ix; ++i) {
			size_t j = 0;
			for (; j < vsz && (*this)[i + j] == v[j]; ++j)
				;
			if (j == vsz) {
				return i;
			}
		}
		return npos;
	}

	size_t find(CharT ch, size_t pos = 0) const {
		return find(basic_string_view(ch, 1), pos);
	}

	size_t find(const CharT *s, size_t pos, size_t count) const {
		return find(basic_string_view(s, count), pos);
	}

	bool operator==(basic_string_view v) const {
		auto sz = size();
		if (size() != v.size()) {
			return false;
		}
		for (size_t i = 0; i < sz; ++i) {
			if ((*this)[i] != v[i]) {
				return false;
			}
		}
		return true;
	}

	bool operator!=(basic_string_view v) const {
		return !(*this == v);
	}

private:
	const CharT *_s;
	size_t _c;
	size_t _max_c;

	size_t _calc_len() const {
		size_t i = 0;
		while (_s[i] != 0) {
			++i;
		}
		return i;
	}
};

template <typename CharT, typename Traits>
std::basic_string<CharT, Traits> &operator+=(
	std::basic_string<CharT, Traits> &a, basic_string_view<CharT, Traits> b) {
	return a += b.data();
}

} // namespace rua

#endif

namespace rua {

using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;

} // namespace rua

#endif
