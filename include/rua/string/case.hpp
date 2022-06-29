#ifndef _RUA_STRING_CASE_HPP
#define _RUA_STRING_CASE_HPP

#include "char_set.hpp"
#include "view.hpp"

#include "../util.hpp"

namespace rua {

RUA_CVAL auto _lower_upper_diff = 'a' - 'A';

inline constexpr char32_t unsafe_to_lower(char32_t c) {
	return c + _lower_upper_diff;
}

inline constexpr char32_t to_lower(char32_t c) {
	return is_upper(c) ? unsafe_to_lower(c) : c;
}

template <
	typename StrLike,
	typename StrView = decltype(view_string(std::declval<StrLike &&>())),
	typename Char = typename StrView::value_type,
	typename Str = std::basic_string<Char> >
inline Str to_lowers(StrLike &&str_like) {
	auto str_v = view_string(std::forward<StrLike>(str_like));
	if (str_v.empty()) {
		return {};
	}
	Str str(str_v);
	for (auto &c : str) {
		if (is_upper(c)) {
			c = static_cast<Char>(unsafe_to_lower(c));
		}
	}
	return str;
}

inline constexpr char32_t unsafe_to_upper(char32_t c) {
	return c - _lower_upper_diff;
}

inline constexpr char32_t to_upper(char32_t c) {
	return is_lower(c) ? unsafe_to_upper(c) : c;
}

template <
	typename StrLike,
	typename StrView = decltype(view_string(std::declval<StrLike &&>())),
	typename Char = typename StrView::value_type,
	typename Str = std::basic_string<Char> >
inline Str to_uppers(StrLike &&str_like) {
	auto str_v = view_string(std::forward<StrLike>(str_like));
	if (str_v.empty()) {
		return {};
	}
	Str str(str_v);
	for (auto &c : str) {
		if (is_lower(c)) {
			c = static_cast<Char>(unsafe_to_upper(c));
		}
	}
	return str;
}

} // namespace rua

#endif
