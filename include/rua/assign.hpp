#ifndef _RUA_ASSIGN_HPP
#define _RUA_ASSIGN_HPP

#include "macros.hpp"

namespace rua {

template <typename Var, typename Val>
RUA_FORCE_INLINE auto assign(Var &&var, Val &&val)
	-> decltype(std::forward<Var>(var) = std::forward<Val>(val)) {
	return std::forward<Var>(var) = std::forward<Val>(val);
}

} // namespace rua

#endif
