#ifndef _rua_string_join_hpp
#define _rua_string_join_hpp

#include "char_set.hpp"
#include "view.hpp"

#include "../range/traits.hpp"
#include "../util.hpp"

#include <string>

namespace rua {

#define RUA_STRING_RANGE(StringRange)                                          \
	typename StringRange = std::initializer_list<string_view>,                 \
			 typename = enable_if_t<std::is_convertible<                       \
				 typename range_traits<StringRange>::element_type,             \
				 string_view>::value>

template <RUA_STRING_RANGE(StrList)>
inline std::string
join(const StrList &strs, string_view sep = "", bool ignore_empty = false) {
	std::string r;

	if (sep.empty()) {
		size_t len = 0;
		for (auto &str : strs) {
			len += str.size();
		}
		if (!len) {
			return r;
		}
		r.reserve(r.size() + len);
		for (auto &str : strs) {
			r += str;
		}
		return r;
	}

	size_t len = 0;
	bool no_add_sep = true;
	for (auto &str : strs) {
		if (ignore_empty && (str.empty() || is_unispaces(str))) {
			continue;
		}
		if (is_controls(str)) {
			no_add_sep = true;
			len += str.size();
			continue;
		}
		if (no_add_sep) {
			no_add_sep = false;
		} else {
			len += sep.size();
		}
		len += str.size();
	}

	if (!len) {
		return r;
	}
	r.reserve(r.size() + len);

	no_add_sep = true;
	for (auto &str : strs) {
		if (ignore_empty && (str.empty() || is_unispaces(str))) {
			continue;
		}
		if (is_controls(str)) {
			no_add_sep = true;
			r += str;
			continue;
		}
		if (no_add_sep) {
			no_add_sep = false;
		} else {
			r += sep;
		}
		r += str;
	}
	return r;
}

template <RUA_STRING_RANGE(StrList)>
inline std::string
join(const StrList &strs, const char sep, bool ignore_empty = false) {
	return join(strs, string_view(&sep, 1), ignore_empty);
}

template <
	typename... Strs,
	typename = decltype(std::initializer_list<string_view>{
		std::declval<Strs &&>()...})>
inline std::string join(Strs &&...strs) {
	return join({strs...});
}

} // namespace rua

#endif
