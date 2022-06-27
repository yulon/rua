#ifndef _RUA_STRING_TRIM_HPP
#define _RUA_STRING_TRIM_HPP

#include "char_set.hpp"
#include "view.hpp"

#include "../util.hpp"

namespace rua {

template <typename StrLike, typename Pred>
inline decltype(slice(std::declval<StrLike &&>()))
trim_if(StrLike &&str_like, Pred &&pred) {
	auto str_v = view(std::forward<StrLike>(str_like));
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

template <typename StrLike>
inline decltype(slice(std::declval<StrLike &&>()))
trim_space(StrLike &&str_like) {
	return trim_if(std::forward<StrLike>(str_like), is_space);
}

template <typename StrLike>
inline decltype(slice(std::declval<StrLike &&>()))
trim_eol(StrLike &&str_like) {
	return trim_if(std::forward<StrLike>(str_like), is_eol);
}

} // namespace rua

#endif
