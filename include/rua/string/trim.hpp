#ifndef _RUA_STRING_TRIM_HPP
#define _RUA_STRING_TRIM_HPP

#include "char_set.hpp"
#include "slice.hpp"
#include "view.hpp"

#include "../util.hpp"

namespace rua {

template <typename StrLike, typename Pred>
inline decltype(slice(std::declval<StrLike &&>()))
trim_if(StrLike &&str_like, Pred &&pred) {
	auto str_v = view_string(std::forward<StrLike>(str_like));
	size_t fst = 0, lst = str_v.length();
	for (size_t i = 0; i < str_v.length(); ++i) {
		if (!std::forward<Pred>(pred)(str_v[i])) {
			fst = i;
			break;
		}
	}
	for (int i = str_v.length() - 1; i >= 0; --i) {
		if (!std::forward<Pred>(pred)(str_v[i])) {
			lst = i + 1;
			break;
		}
	}
	return slice(std::forward<StrLike>(str_like), fst, lst);
}

template <typename StrLike, typename Pred>
inline decltype(slice(std::declval<StrLike &&>()))
trim_left_if(StrLike &&str_like, Pred &&pred) {
	auto str_v = view_string(std::forward<StrLike>(str_like));
	size_t fst = 0;
	for (size_t i = 0; i < str_v.length(); ++i) {
		if (!std::forward<Pred>(pred)(str_v[i])) {
			fst = i;
			break;
		}
	}
	return slice(std::forward<StrLike>(str_like), fst);
}

template <typename StrLike, typename Pred>
inline decltype(slice(std::declval<StrLike &&>()))
trim_right_if(StrLike &&str_like, Pred &&pred) {
	auto str_v = view_string(std::forward<StrLike>(str_like));
	size_t lst = str_v.length();
	for (auto i = to_signed(str_v.length() - 1); i >= 0; --i) {
		if (!std::forward<Pred>(pred)(str_v[i])) {
			lst = to_unsigned(i + 1);
			break;
		}
	}
	return slice(std::forward<StrLike>(str_like), 0, lst);
}

template <typename StrLike>
inline decltype(slice(std::declval<StrLike &&>()))
trim_space(StrLike &&str_like) {
	return trim_if(std::forward<StrLike>(str_like), is_space);
}

template <typename StrLike>
inline decltype(slice(std::declval<StrLike &&>()))
trim_left_space(StrLike &&str_like) {
	return trim_left_if(std::forward<StrLike>(str_like), is_space);
}

template <typename StrLike>
inline decltype(slice(std::declval<StrLike &&>()))
trim_right_space(StrLike &&str_like) {
	return trim_right_if(std::forward<StrLike>(str_like), is_space);
}

template <typename StrLike>
inline decltype(slice(std::declval<StrLike &&>()))
trim_eol(StrLike &&str_like) {
	return trim_if(std::forward<StrLike>(str_like), is_eol);
}

template <typename StrLike>
inline decltype(slice(std::declval<StrLike &&>()))
trim_left_eol(StrLike &&str_like) {
	return trim_left_if(std::forward<StrLike>(str_like), is_eol);
}

template <typename StrLike>
inline decltype(slice(std::declval<StrLike &&>()))
trim_right_eol(StrLike &&str_like) {
	return trim_right_if(std::forward<StrLike>(str_like), is_eol);
}

} // namespace rua

#endif
