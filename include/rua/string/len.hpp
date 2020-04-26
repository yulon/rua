#ifndef _RUA_STRING_LEN_HPP
#define _RUA_STRING_LEN_HPP

#include "../macros.hpp"
#include "../types/util.hpp"

namespace rua {

#ifdef RUA_CONSTEXPR_14_SUPPORTED

template <typename CharT>
inline constexpr size_t c_str_len_us(const CharT *c_str) {
	size_t i = 0;
	while (c_str[i] != 0) {
		++i;
	}
	return i;
}

#else

template <typename CharT>
inline constexpr size_t _c_str_len_us(const CharT *begin, const CharT *end) {
	return *end ? _c_str_len_us<CharT>(begin, ++end) : end - begin;
}

template <typename CharT>
inline constexpr size_t c_str_len_us(const CharT *c_str) {
	return _c_str_len_us<CharT>(c_str, c_str);
}

#endif

template <typename CharT>
inline constexpr size_t c_str_len(const CharT *c_str) {
	return c_str ? c_str_len_us<CharT>(c_str) : 0;
}

} // namespace rua

#endif
