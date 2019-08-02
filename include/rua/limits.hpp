#ifndef _RUA_LIMITS_HPP
#define _RUA_LIMITS_HPP

#include <limits>

#include "switch_nonstd_macros.h"

namespace rua {

template <typename T>
constexpr T nmax() {
	return std::numeric_limits<T>::max();
}

template <typename T>
constexpr T nmin() {
	return std::numeric_limits<T>::min();
}

template <typename T>
constexpr T nlowest() {
	return std::numeric_limits<T>::lowest();
}

} // namespace rua

#include "switch_nonstd_macros.h"

#endif
