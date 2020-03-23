#ifndef _RUA_TYPES_TRAITS_HPP
#define _RUA_TYPES_TRAITS_HPP

#include "util.hpp"

#include <type_traits>

namespace rua {

// Non-standard

template <bool Cond, typename T, typename... Args>
struct rebind_if;

template <typename T, typename... Args>
struct rebind_if<true, T, Args...> {
	using type = typename T::template rebind<Args...>::type;
};

template <typename T, typename... Args>
struct rebind_if<false, T, Args...> {
	using type = T;
};

template <bool Cond, typename T, typename... Args>
using rebind_if_t = typename rebind_if<Cond, T, Args...>::type;

template <bool Cond, typename... T>
struct enable_if;

template <>
struct enable_if<true> {
	using type = void;
};

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

template <bool Cond, typename... T>
using enable_if_t = typename enable_if<Cond, T...>::type;

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

template <bool Cond, typename T, typename... F>
using conditional_t = typename conditional<Cond, T, F...>::type;

template <typename FirstArg, typename... Args>
struct argments_front {
	using type = FirstArg;
};

template <typename... Args>
using argments_front_t = typename argments_front<Args...>::type;

// Standard alias

#ifdef __cpp_lib_bool_constant

template <bool Cond>
using bool_constant = std::bool_constant<Cond>;

#else

template <bool Cond>
using bool_constant = std::integral_constant<bool, Cond>;

#endif

template <typename T>
using remove_cv_t = typename std::remove_cv<T>::type;

template <typename T>
using remove_const_t = typename std::remove_const<T>::type;

template <typename T>
using remove_volatile_t = typename std::remove_volatile<T>::type;

template <typename T>
using add_cv_t = typename std::add_cv<T>::type;

template <typename T>
using add_const_t = typename std::add_const<T>::type;

template <typename T>
using add_volatile_t = typename std::add_volatile<T>::type;

template <typename T>
using remove_reference_t = typename std::remove_reference<T>::type;

template <typename T>
using add_lvalue_reference_t = typename std::add_lvalue_reference<T>::type;

template <typename T>
using add_rvalue_reference_t = typename std::add_rvalue_reference<T>::type;

template <typename T>
using remove_pointer_t = typename std::remove_pointer<T>::type;

template <typename T>
using add_pointer_t = typename std::add_pointer<T>::type;

template <typename T>
using make_signed_t = typename std::make_signed<T>::type;

template <typename T>
using make_unsigned_t = typename std::make_unsigned<T>::type;

template <typename T>
using remove_extent_t = typename std::remove_extent<T>::type;

template <typename T>
using remove_all_extents_t = typename std::remove_all_extents<T>::type;

template <std::size_t Len, std::size_t Align = Len>
using aligned_storage_t = typename std::aligned_storage<Len, Align>::type;

template <typename T>
using decay_t = typename std::decay<T>::type;

template <typename... T>
using common_type_t = typename std::common_type<T...>::type;

template <typename T>
using underlying_type_t = typename std::underlying_type<T>::type;

// Standard Compatibility

template <typename...>
using void_t = void;

template <typename T>
struct is_bounded_array : std::false_type {};

template <typename T, size_t N>
struct is_bounded_array<T[N]> : std::true_type {};

template <typename T>
struct is_unbounded_array : std::false_type {};

template <typename T>
struct is_unbounded_array<T[]> : std::true_type {};

#ifdef __cpp_lib_is_null_pointer

template <typename T>
using is_null_pointer = std::is_null_pointer<T>;

#else

template <typename T>
struct is_null_pointer : std::is_same<remove_cv_t<T>, std::nullptr_t> {};

#endif

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

#ifdef __cpp_lib_remove_cvref

template <typename T>
using remove_cvref = std::remove_cvref<T>;

#else

template <typename T>
struct remove_cvref : std::remove_cv<remove_reference_t<T> > {};

#endif

template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

} // namespace rua

#endif
