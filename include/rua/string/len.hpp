#ifndef _rua_string_len_hpp
#define _rua_string_len_hpp

#include "../util.hpp"

namespace rua {

#ifdef RUA_HAS_CONSTEXPR_14

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
