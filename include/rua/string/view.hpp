#ifndef _RUA_STRING_VIEW_HPP
#define _RUA_STRING_VIEW_HPP

#include "../macros.hpp"

#ifdef __cpp_lib_string_view

#include <string_view>

namespace rua {

template <typename CharT, typename Traits = std::char_traits<CharT> >
using basic_string_view = std::basic_string_view<CharT, Traits>;

}

#else

#include "len.hpp"

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

	constexpr basic_string_view(const CharT *s, size_type count) :
		_s(s), _c(count) {}

	constexpr basic_string_view(const CharT *s) : _s(s), _c(c_str_len(s)) {}

	basic_string_view(const std::basic_string<CharT, Traits> &s) :
		basic_string_view(s.c_str(), s.length()) {}

	constexpr size_type length() const {
		return _c;
	}

	constexpr size_type size() const {
		return _c;
	}

	constexpr size_type max_size() const {
		return static_cast<size_type>(-1);
	}

	constexpr const CharT *data() const {
		return _s;
	}

	constexpr bool empty() const {
		return !length();
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
		return begin() + length();
	}

	constexpr const CharT *cend() const {
		return cbegin() + length();
	}

	constexpr basic_string_view substr(size_t pos, size_t len) const {
		return basic_string_view(data() + pos, len);
	}

	constexpr basic_string_view substr(size_t pos) const {
		return substr(pos, length() - pos);
	}

	RUA_CONSTEXPR_14 bool operator==(basic_string_view v) const {
		auto sz = length();
		if (length() != v.length()) {
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

	static constexpr auto npos = static_cast<size_type>(-1);

	RUA_CONSTEXPR_14 size_type
	find(basic_string_view sub, size_type pos = 0) const {
		auto end = length() - sub.length();
		for (size_type i = pos; i < end; ++i) {
			if (substr(i, sub.length()) == sub) {
				return i;
			}
		}
		return npos;
	}

	RUA_CONSTEXPR_14 size_type find(CharT ch, size_type pos = 0) const {
		return find({&ch, 1}, pos);
	}

	RUA_CONSTEXPR_14 size_type
	find(const CharT *s, size_type pos, size_type count) const {
		return find({s, count}, pos);
	}

	RUA_CONSTEXPR_14 size_type
	rfind(basic_string_view sub, size_type pos = npos) const {
		if (pos == npos) {
			pos = length() - sub.length();
		}
		for (auto i = to_signed(pos); i >= 0; --i) {
			if (substr(i, sub.length()) == sub) {
				return i;
			}
		}
		return npos;
	}

	RUA_CONSTEXPR_14 size_type rfind(CharT ch, size_type pos = npos) const {
		return rfind({&ch, 1}, pos);
	}

	RUA_CONSTEXPR_14 size_type
	rfind(const CharT *s, size_type pos, size_type count) const {
		return rfind({s, count}, pos);
	}

	explicit operator std::basic_string<CharT, Traits>() const {
		return {data(), length()};
	}

private:
	const CharT *_s;
	size_type _c;
};

template <typename CharT, typename Traits>
std::basic_string<CharT, Traits> &operator+=(
	std::basic_string<CharT, Traits> &a, basic_string_view<CharT, Traits> b) {
	return a += std::basic_string<CharT, Traits>(b);
}

} // namespace rua

#endif

namespace rua {

using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;

} // namespace rua

#endif
