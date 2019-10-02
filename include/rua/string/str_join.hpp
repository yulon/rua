#ifndef _RUA_STRING_STR_JOIN_HPP
#define _RUA_STRING_STR_JOIN_HPP

#include "line.hpp"
#include "string_view.hpp"

#include "../type_traits/std_patch.hpp"

#include <cstdint>
#include <initializer_list>
#include <string>

namespace rua {

static constexpr uint32_t str_join_ignore_empty = 0x0000000F;
static constexpr uint32_t str_join_multi_line = 0x000000F0;

template <
	typename StrList,
	typename = enable_if_t<
		std::is_convertible<typename StrList::value_type, string_view>::value>>
inline void str_join(
	std::string &buf,
	const StrList &strs,
	string_view sep = "",
	uint32_t flags = 0,
	size_t pre_reserved_size = 0) {

	if (sep.empty()) {
		size_t len = 0;
		for (auto &str : strs) {
			len += str.size();
		}
		if (!len) {
			return;
		}
		buf.reserve(buf.size() + len + pre_reserved_size);
		for (auto &str : strs) {
			buf += str;
		}
		return;
	}

	bool is_ignore_empty = flags & str_join_ignore_empty;
	bool is_multi_line = flags & str_join_multi_line;

	size_t len = 0;
	bool bf_is_eol = true;
	for (auto &str : strs) {
		if (!str.size() && (is_ignore_empty)) {
			continue;
		}
		if (bf_is_eol) {
			bf_is_eol = false;
		} else {
			len += sep.size();
		}
		len += str.size();
		if (is_multi_line && is_eol(str)) {
			bf_is_eol = true;
		}
	}

	if (!len) {
		return;
	}
	buf.reserve(buf.size() + len + pre_reserved_size);

	bf_is_eol = true;
	for (auto &str : strs) {
		if (!str.size() && is_ignore_empty) {
			continue;
		}
		if (bf_is_eol) {
			bf_is_eol = false;
		} else {
			buf += sep;
		}
		buf += str;
		if (is_multi_line && is_eol(str)) {
			bf_is_eol = true;
		}
	}
	return;
}

template <
	typename StrList,
	typename = enable_if_t<
		std::is_convertible<typename StrList::value_type, string_view>::value>>
inline std::string str_join(
	const StrList &strs,
	string_view sep = "",
	uint32_t flags = 0,
	size_t pre_reserved_size = 0) {
	std::string r;
	str_join(r, strs, sep, flags, pre_reserved_size);
	return r;
}

template <
	typename... Strs,
	typename StrList = std::initializer_list<string_view>,
	typename = decltype(StrList{std::declval<Strs>()...})>
inline std::string str_join(Strs &&... strs) {
	return str_join(StrList{strs...});
}

} // namespace rua

#endif
