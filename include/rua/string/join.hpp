#ifndef _RUA_STRING_JOIN_HPP
#define _RUA_STRING_JOIN_HPP

#include "char_set.hpp"
#include "view.hpp"

#include "../macros.hpp"
#include "../types/traits.hpp"
#include "../types/util.hpp"

#include <string>

namespace rua {

struct str_join_options {
	bool is_ignore_space = false;
	size_t pre_reserved_size = 0;
};

template <
	typename StrList = std::initializer_list<string_view>,
	typename = enable_if_t<std::is_convertible<
		decltype(*std::declval<const StrList &>().begin()),
		string_view>::value>>
inline void str_join(
	std::string &buf,
	const StrList &strs,
	string_view sep = "",
	const str_join_options &opt = {}) {

	if (sep.empty()) {
		size_t len = 0;
		for (auto &str : strs) {
			len += str.size();
		}
		if (!len) {
			return;
		}
		buf.reserve(buf.size() + len + opt.pre_reserved_size);
		for (auto &str : strs) {
			buf += str;
		}
		return;
	}

	size_t len = 0;
	bool no_add_sep = true;
	for (auto &str : strs) {
		if (opt.is_ignore_space && is_space(str)) {
			continue;
		}
		if (is_control(str)) {
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
		return;
	}
	buf.reserve(buf.size() + len + opt.pre_reserved_size);

	no_add_sep = true;
	for (auto &str : strs) {
		if (!str.size() && opt.is_ignore_space) {
			continue;
		}
		if (is_control(str)) {
			no_add_sep = true;
			buf += str;
			continue;
		}
		if (no_add_sep) {
			no_add_sep = false;
		} else {
			buf += sep;
		}
		buf += str;
	}
	return;
}

template <
	typename StrList = std::initializer_list<string_view>,
	typename = enable_if_t<std::is_convertible<
		decltype(*std::declval<const StrList &>().begin()),
		string_view>::value>>
inline void str_join(
	std::string &buf,
	const StrList &strs,
	const char sep,
	const str_join_options &opt = {}) {
	str_join(buf, strs, string_view(&sep, 1), opt);
}

template <
	typename StrList = std::initializer_list<string_view>,
	typename = enable_if_t<std::is_convertible<
		decltype(*std::declval<const StrList &>().begin()),
		string_view>::value>>
inline std::string str_join(
	const StrList &strs,
	string_view sep = "",
	const str_join_options &opt = {}) {
	std::string r;
	str_join(r, strs, sep, opt);
	return r;
}

template <
	typename StrList = std::initializer_list<string_view>,
	typename = enable_if_t<std::is_convertible<
		decltype(*std::declval<const StrList &>().begin()),
		string_view>::value>>
inline std::string str_join(
	const StrList &strs, const char sep, const str_join_options &opt = {}) {
	std::string r;
	str_join(r, strs, sep, opt);
	return r;
}

template <
	typename... Strs,
	typename = decltype(std::initializer_list<string_view>{
		std::declval<Strs &&>()...})>
inline std::string str_join(Strs &&... strs) {
	return str_join({strs...});
}

} // namespace rua

#endif
