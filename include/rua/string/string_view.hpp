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

#include "str_len.hpp"

#include "../types/util.hpp"

#include <cstring>
#include <string>

namespace rua {

template <typename CharT, typename Traits = std::char_traits<CharT> >
class basic_string_view {
public:
	using size_type = size_t;
	using value_type = CharT;
	using iterator = const CharT *;
	using const_iterator = const CharT *;

	constexpr basic_string_view() : _s(nullptr), _c(0) {}

	constexpr basic_string_view(std::nullptr_t) : basic_string_view() {}

#ifdef RUA_CONSTEXPR_14_SUPPORTED
	constexpr basic_string_view(const CharT *s, size_type count) :
		_s(s), _c(count) {}
	constexpr basic_string_view(const CharT *s) : _s(s), _c(str_len(s)) {}
#else
	static constexpr auto _nullsz = static_cast<size_type>(-3);
	constexpr basic_string_view(const CharT *s, size_type count = _nullsz) :
		_s(s), _c(count) {}
#endif

	basic_string_view(const std::basic_string<CharT, Traits> &s) :
		basic_string_view(s.c_str(), s.size()) {}

	RUA_CONSTEXPR_14 size_type size() {
#ifndef RUA_CONSTEXPR_14_SUPPORTED
		if (_c == _nullsz) {
			_c = str_len(_s);
		}
#endif
		return _c;
	}

	RUA_CONSTEXPR_14 size_type size() const {
#ifndef RUA_CONSTEXPR_14_SUPPORTED
		if (_c == _nullsz) {
			return str_len(_s);
		}
#endif
		return _c;
	}

	constexpr size_type max_size() const {
		return static_cast<size_type>(-1);
	}

	RUA_CONSTEXPR_14 size_type length() {
		return size();
	}

	RUA_CONSTEXPR_14 size_type length() const {
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

	static constexpr auto npos = static_cast<size_type>(-1);

	RUA_CONSTEXPR_14 size_type
	find(basic_string_view v, size_type pos = 0) const {
		auto vsz = v.size();
		auto end_ix = size() - vsz + 1;
		for (size_type i = pos; i < end_ix; ++i) {
			size_type j = 0;
			for (; j < vsz && (*this)[i + j] == v[j]; ++j)
				;
			if (j == vsz) {
				return i;
			}
		}
		return npos;
	}

	RUA_CONSTEXPR_14 size_type find(CharT ch, size_type pos = 0) const {
		return find(basic_string_view(&ch, 1), pos);
	}

	RUA_CONSTEXPR_14 size_type
	find(const CharT *s, size_type pos, size_type count) const {
		return find(basic_string_view(s, count), pos);
	}

	RUA_CONSTEXPR_14 bool operator==(basic_string_view v) const {
		auto sz = size();
		if (size() != v.size()) {
			return false;
		}
		for (size_type i = 0; i < sz; ++i) {
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
	size_type _c;
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
