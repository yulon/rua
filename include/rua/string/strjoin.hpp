#ifndef _RUA_STRING_STRJOIN_HPP
#define _RUA_STRING_STRJOIN_HPP

#include "line.hpp"

#include "../stldata.hpp"

#include <string>

#ifdef __cpp_lib_string_view
#include <string_view>
#endif

#include <cstdint>
#include <vector>

namespace rua {

static constexpr uint32_t strjoin_ignore_empty = 0x0000000F;
static constexpr uint32_t strjoin_multi_line = 0x000000F0;

inline std::string strjoin(
	const std::vector<std::string> &strs,
	const std::string &sep,
	uint32_t flags = 0,
	size_t reserved_size = 0) {
	bool is_ignore_empty = flags & strjoin_ignore_empty;
	bool is_multi_line = flags & strjoin_multi_line;

	size_t len = 0;
	bool bf_is_eol = true;
	for (auto &str : strs) {
		if (!str.length() && is_ignore_empty) {
			continue;
		}
		if (bf_is_eol) {
			bf_is_eol = false;
		} else {
			len += sep.length();
		}
		len += str.length();
		if (is_multi_line && is_eol(str)) {
			bf_is_eol = true;
		}
	}

	std::string r;
	if (!len) {
		return r;
	}
	r.reserve(len + reserved_size);
	r.resize(len);

	size_t pos = 0;
	bf_is_eol = true;
	for (auto &str : strs) {
		if (!str.length() && is_ignore_empty) {
			continue;
		}
		if (bf_is_eol) {
			bf_is_eol = false;
		} else {
			sep.copy(stldata(r) + pos, sep.length());
			pos += sep.length();
		}
		str.copy(stldata(r) + pos, str.length());
		pos += str.length();
		if (is_multi_line && is_eol(str)) {
			bf_is_eol = true;
		}
	}
	return r;
}

} // namespace rua

#endif
