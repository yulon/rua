#ifndef _RUA_TYPE_TRAITS_STD_PATCH_HPP
#define _RUA_TYPE_TRAITS_STD_PATCH_HPP

#include "../macros.hpp"

#include <cstddef>
#include <type_traits>

namespace rua {

// Compatibility and improvement of <type_traits>.

#ifdef __cpp_lib_bool_constant

template <bool Cond>
using bool_constant = std::bool_constant<Cond>;

#else

template <bool Cond>
using bool_constant = std::integral_constant<bool, Cond>;

#endif

template <bool Cond, typename... T>
struct enable_if;

template <typename T>
struct enable_if<true, T> {
	using type = T;
};

template <typename T, typename... TArgs>
struct enable_if<true, T, TArgs...> {
	using type = typename T::template rebind<TArgs...>::type;
};

template <typename... T>
struct enable_if<false, T...> {};

template <bool Cond, typename T, typename... F>
struct conditional;

template <typename T, typename... F>
struct conditional<true, T, F...> {
	using type = T;
};

template <typename T, typename F>
struct conditional<false, T, F> {
	using type = F;
};

template <typename T, typename F, typename... FArgs>
struct conditional<false, T, F, FArgs...> {
	using type = typename F::template rebind<FArgs...>::type;
};

#ifdef __cpp_lib_type_identity

template <typename T>
using type_identity = std::type_identity<T>;

#else

template <typename T>
struct type_identity {
	using type = T;
};

#endif

template <typename T>
using type_identity_t = typename type_identity<T>::type;

} // namespace rua

#if RUA_CPP >= RUA_CPP_17 || defined(__cpp_lib_optional) ||                    \
	defined(__cpp_lib_variant) || defined(__cpp_lib_any)

#include <utility>

namespace rua {

using in_place_t = std::in_place_t;

template <typename T>
using in_place_type_t = std::in_place_type_t<T>;

template <size_t I>
using in_place_index_t = std::in_place_index_t<I>;

} // namespace rua

#else

#include <cstddef>

namespace rua {

struct in_place_t {};

template <typename T>
struct in_place_type_t {};

template <size_t I>
struct in_place_index_t {};

} // namespace rua

#endif

namespace rua {

RUA_MULTIDEF_VAR constexpr in_place_t in_place{};

} // namespace rua

#endif
