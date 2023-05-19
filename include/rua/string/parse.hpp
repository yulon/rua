#ifndef _rua_string_parse_hpp
#define _rua_string_parse_hpp

#include "char_set.hpp"
#include "view.hpp"

#include "../span.hpp"
#include "../util.hpp"

namespace rua {

template <typename StrSpan, typename = span_traits<StrSpan &&>>
inline bool skip_space(StrSpan &&ss, size_t &i) {
	while (is_unispace(data(std::forward<StrSpan>(ss))[i])) {
		++i;
		if (i >= size(std::forward<StrSpan>(ss))) {
			return false;
		}
	};
	return true;
}

} // namespace rua

#endif
