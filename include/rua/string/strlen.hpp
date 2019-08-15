#ifndef _RUA_STRING_STRLEN_HPP
#define _RUA_STRING_STRLEN_HPP

#include "../macros.hpp"

namespace rua {

template <typename CharT>
static RUA_CONSTEXPR_14 size_t strlen(const CharT *c_str) {
	size_t i = 0;
	while (c_str[i] != 0) {
		++i;
	}
	return i;
}

} // namespace rua

#endif
