#ifndef _RUA_STRING_WITH_HPP
#define _RUA_STRING_WITH_HPP

#include "view.hpp"

#include "../util.hpp"

namespace rua {

template <
	typename StrLike,
	typename StrView = decltype(view_string(std::declval<StrLike &&>()))>
inline RUA_CONSTEXPR_14 bool
starts_with(StrLike &&str_like, type_identity_t<StrView> search_str_v) {
	StrView str_v(std::forward<StrLike>(str_like));
	if (str_v.length() < search_str_v.length()) {
		return false;
	}
	return str_v.substr(0, search_str_v.length()) == search_str_v;
}

template <
	typename StrLike,
	typename StrView = decltype(view_string(std::declval<StrLike &&>()))>
inline RUA_CONSTEXPR_14 bool
ends_with(StrLike &&str_like, type_identity_t<StrView> search_str_v) {
	StrView str_v(std::forward<StrLike>(str_like));
	if (str_v.length() < search_str_v.length()) {
		return false;
	}
	return str_v.substr(
			   str_v.length() - search_str_v.length(), search_str_v.length()) ==
		   search_str_v;
}

} // namespace rua

#endif
