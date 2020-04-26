#ifndef _RUA_STRING_LEN_HPP
#define _RUA_STRING_LEN_HPP

#include "../macros.hpp"
#include "../types/util.hpp"

namespace rua {

template <typename CharT>
inline RUA_CONSTEXPR_14 size_t c_str_len_us(const CharT *c_str) {
	size_t i = 0;
	while (c_str[i] != 0) {
		++i;
	}
	return i;
}

template <typename CharT>
inline RUA_CONSTEXPR_14 size_t c_str_len(const CharT *c_str) {
	if (!c_str) {
		return 0;
	}
	return c_str_len_us<CharT>(c_str);
}

} // namespace rua

#endif
