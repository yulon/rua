#ifndef _RUA_STRING_WITH_HPP
#define _RUA_STRING_WITH_HPP

#include "view.hpp"

#include "../util.hpp"

namespace rua {

inline RUA_CONSTEXPR_14 bool
starts_with(string_view str, string_view search_str) {
	if (str.length() < search_str.length()) {
		return false;
	}
	return str.substr(0, search_str.length()) == search_str;
}

inline RUA_CONSTEXPR_14 bool
ends_with(string_view str, string_view search_str) {
	if (str.length() < search_str.length()) {
		return false;
	}
	return str.substr(
			   str.length() - search_str.length(), search_str.length()) ==
		   search_str;
}

} // namespace rua

#endif
