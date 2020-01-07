#ifndef _RUA_LIMITS_HPP
#define _RUA_LIMITS_HPP

#include "macros.hpp"

#include <limits>

namespace rua {

template <typename T>
RUA_FORCE_INLINE constexpr T nmax() {
	return (std::numeric_limits<T>::max)();
}

template <typename T>
RUA_FORCE_INLINE constexpr T nmin() {
	return (std::numeric_limits<T>::min)();
}

template <typename T>
RUA_FORCE_INLINE constexpr T nlowest() {
	return (std::numeric_limits<T>::lowest)();
}

} // namespace rua

#endif
