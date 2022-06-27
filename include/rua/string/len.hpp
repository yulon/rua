#ifndef _RUA_STRING_LEN_HPP
#define _RUA_STRING_LEN_HPP

#include "../util.hpp"

namespace rua {

#ifdef RUA_CONSTEXPR_14_SUPPORTED

template <typename CharT>
inline constexpr size_t unsafe_c_str_len(const CharT *c_str) {
	size_t i = 0;
	while (c_str[i] != 0) {
		++i;
	}
	return i;
}

#else

template <typename CharT>
inline constexpr size_t
_unsafe_c_str_len(const CharT *begin, const CharT *end) {
	return *end ? _unsafe_c_str_len<CharT>(begin, end + 1) : end - begin;
}

template <typename CharT>
inline constexpr size_t unsafe_c_str_len(const CharT *c_str) {
	return _unsafe_c_str_len<CharT>(c_str, c_str);
}

#endif

template <typename CharT>
inline constexpr size_t c_str_len(const CharT *c_str) {
	return c_str ? unsafe_c_str_len<CharT>(c_str) : 0;
}

} // namespace rua

#endif
