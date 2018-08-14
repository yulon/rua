#ifndef _RUA_LIMITS_HPP
#define _RUA_LIMITS_HPP

#include <limits>

#include "disable_msvc_sh1t.h"

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
}

#include "enable_msvc_sh1t.h"

#endif
