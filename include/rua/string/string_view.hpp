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
	constexpr basic_string_view() : _s(nullptr), _c(0) {}

	constexpr basic_string_view(std::nullptr_t) : basic_string_view() {}

#ifdef RUA_CONSTEXPR_14_SUPPORTED
	constexpr basic_string_view(const CharT *s, size_t count) :
		_s(s),
		_c(count) {}
	constexpr basic_string_view(const CharT *s) : _s(s), _c(_strlen(s)) {}
#else
	static constexpr auto _nullsz = static_cast<size_t>(-3);
	constexpr basic_string_view(const CharT *s, size_t count = _nullsz) :
		_s(s),
		_c(count) {}
#endif

	basic_string_view(const std::basic_string<CharT, Traits> &s) :
		basic_string_view(s.c_str(), s.size()) {}

	RUA_CONSTEXPR_14 size_t size() {
#ifndef RUA_CONSTEXPR_14_SUPPORTED
		if (_c == _nullsz) {
			_c = _strlen(_s);
		}
#endif
		return _c;
	}

	RUA_CONSTEXPR_14 size_t size() const {
#ifndef RUA_CONSTEXPR_14_SUPPORTED
		if (_c == _nullsz) {
			return _strlen(_s);
		}
#endif
		return _c;
	}

	constexpr size_t max_size() const {
		return static_cast<size_t>(-1);
	}

	RUA_CONSTEXPR_14 size_t length() {
		return size();
	}

	RUA_CONSTEXPR_14 size_t length() const {
		return size();
	}

	constexpr const CharT *data() const {
		return _s;
	}

	RUA_CONSTEXPR_14 bool empty() const {
		return !size();
	}

	constexpr const CharT &operator[](ptrdiff_t ix) const {
		return _s[ix];
	}

	constexpr const CharT *begin() const {
		return data();
	}

	constexpr const CharT *cbegin() const {
		return data();
	}

	constexpr const CharT *end() const {
		return begin() + size();
	}

	constexpr const CharT *cend() const {
		return cbegin() + size();
	}

	static constexpr size_t npos = static_cast<size_t>(-1);

	RUA_CONSTEXPR_14 size_t find(basic_string_view v, size_t pos = 0) const {
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

	RUA_CONSTEXPR_14 size_t find(CharT ch, size_t pos = 0) const {
		return find(basic_string_view(&ch, 1), pos);
	}

	RUA_CONSTEXPR_14 size_t
	find(const CharT *s, size_t pos, size_t count) const {
		return find(basic_string_view(s, count), pos);
	}

	RUA_CONSTEXPR_14 bool operator==(basic_string_view v) const {
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

	RUA_CONSTEXPR_14 bool operator!=(basic_string_view v) const {
		return !(*this == v);
	}

private:
	const CharT *_s;
	size_t _c;

	static RUA_CONSTEXPR_14 size_t _strlen(const CharT *c_str) {
		size_t i = 0;
		while (c_str[i] != 0) {
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
