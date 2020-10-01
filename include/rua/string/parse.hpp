#ifndef _RUA_STRING_PARSE_HPP
#define _RUA_STRING_PARSE_HPP

#include "char_set.hpp"
#include "view.hpp"

#include "../io/abstract.hpp"
#include "../span.hpp"
#include "../types/util.hpp"

namespace rua {

template <typename StrSpan, typename = span_traits<StrSpan &&>>
inline bool skip_space(StrSpan &&ss, size_t &i) {
	while (is_space(data(std::forward<StrSpan>(ss))[i])) {
		++i;
		if (i >= size(std::forward<StrSpan>(ss))) {
			return false;
		}
	};
	return true;
}

} // namespace rua

#endif
