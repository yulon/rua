#ifndef _RUA_STRING_STRJOIN_HPP
#define _RUA_STRING_STRJOIN_HPP

#include "line.hpp"
#include "string_view.hpp"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <type_traits>

namespace rua {

static constexpr uint32_t strjoin_ignore_empty = 0x0000000F;
static constexpr uint32_t strjoin_multi_line = 0x000000F0;

template <
	typename StrList,
	typename = typename std::enable_if<
		std::is_convertible<typename StrList::value_type, string_view>::value>::
		type>
inline void strjoin(
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

	bool is_ignore_empty = flags & strjoin_ignore_empty;
	bool is_multi_line = flags & strjoin_multi_line;

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
	typename = typename std::enable_if<
		std::is_convertible<typename StrList::value_type, string_view>::value>::
		type>
inline std::string strjoin(
	const StrList &strs,
	string_view sep = "",
	uint32_t flags = 0,
	size_t pre_reserved_size = 0) {
	std::string r;
	strjoin(r, strs, sep, flags, pre_reserved_size);
	return r;
}

template <
	typename... Strs,
	typename StrList = std::initializer_list<string_view>,
	typename = decltype(StrList{std::declval<Strs>()...})>
inline std::string strjoin(Strs &&... strs) {
	return strjoin(StrList{strs...});
}

} // namespace rua

#endif
