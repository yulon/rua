#ifndef _RUA_TYPE_TRAITS_MEASUREMENTS_HPP
#define _RUA_TYPE_TRAITS_MEASUREMENTS_HPP

#include "../macros.hpp"

#include <cstddef>

namespace rua {

template <typename T>
struct size_of {
	static constexpr size_t value = sizeof(T);
};

template <>
struct size_of<void> {
	static constexpr size_t value = 0;
};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto size_of_v = size_of<T>::value;
#endif

template <typename T>
struct align_of {
	static constexpr size_t value = alignof(T);
};

template <>
struct align_of<void> {
	static constexpr size_t value = 0;
};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto align_of_v = align_of<T>::value;
#endif

} // namespace rua

#endif