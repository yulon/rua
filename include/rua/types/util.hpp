#ifndef _RUA_TYPES_UTIL_HPP
#define _RUA_TYPES_UTIL_HPP

#include "traits.hpp"

#include "../macros.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <utility>

#define RUA_CONTAINER_OF(member_ptr, type, member)                             \
	reinterpret_cast<type *>(                                                  \
		reinterpret_cast<uintptr_t>(member_ptr) - offsetof(type, member))

#define RUA_IS_BASE_OF_CONCEPT(_B, _D)                                         \
	typename _D,                                                               \
		typename =                                                             \
			enable_if_t<std::is_base_of<_B, remove_reference_t<_D>>::value>

#define RUA_DERIVED_CONCEPT(_B, _D)                                            \
	typename _D,                                                               \
		typename = enable_if_t <                                               \
					   std::is_base_of<_B, remove_reference_t<_D>>::value &&   \
				   !std::is_same<_B, _D>::value >

#define RUA_OVERLOAD_ASSIGNMENT_L(T)                                           \
	T &operator=(const T &src) {                                               \
		if (this == &src) {                                                    \
			T tmp(src);                                                        \
			return operator=(std::move(tmp));                                  \
		}                                                                      \
		this->~T();                                                            \
		new (this) T(src);                                                     \
		return *this;                                                          \
	}

#define RUA_OVERLOAD_ASSIGNMENT_R(T)                                           \
	T &operator=(T &&src) {                                                    \
		if (this == &src) {                                                    \
			return *this;                                                      \
		}                                                                      \
		this->~T();                                                            \
		new (this) T(std::move(src));                                          \
		return *this;                                                          \
	}

#define RUA_OVERLOAD_ASSIGNMENT(T)                                             \
	RUA_OVERLOAD_ASSIGNMENT_L(T)                                               \
	RUA_OVERLOAD_ASSIGNMENT_R(T)

#if defined(__cpp_rtti) && __cpp_rtti
#define RUA_RTTI __cpp_rtti
#elif defined(_HAS_STATIC_RTTI) && _HAS_STATIC_RTTI
#define RUA_RTTI _HAS_STATIC_RTTI
#endif

#define RUA_DI(T, ...)                                                         \
	([&]() -> T {                                                              \
		T $;                                                                   \
		__VA_ARGS__;                                                           \
		return $;                                                              \
	}())

#if RUA_CPP >= RUA_CPP_17
#define RUA_WEAK_FROM_THIS weak_from_this()
#else
#define RUA_WEAK_FROM_THIS shared_from_this()
#endif

namespace rua {

using uint = unsigned int;
using ushort = unsigned short;
using schar = signed char;
using uchar = unsigned char;

////////////////////////////////////////////////////////////////////////////

template <typename T>
inline constexpr T nmax() {
	return (std::numeric_limits<T>::max)();
}

template <typename T>
inline constexpr bool is_max(T n) {
	return n == nmax<T>();
}

template <typename T>
inline constexpr T nmin() {
	return (std::numeric_limits<T>::min)();
}

template <typename T>
inline constexpr bool is_min(T n) {
	return n == nmin<T>();
}

template <typename T>
inline constexpr T nlowest() {
	return (std::numeric_limits<T>::lowest)();
}

template <typename T>
inline constexpr bool is_lowest(T n) {
	return n == nlowest<T>();
}

////////////////////////////////////////////////////////////////////////////

RUA_INLINE_CONST auto nullpos = nmax<size_t>();

////////////////////////////////////////////////////////////////////////////

template <typename A, typename B>
inline constexpr decltype(std::declval<A &&>() = std::declval<B &&>())
assign(A &&a, B &&b) {
	return std::forward<A>(a) = std::forward<B>(b);
}

////////////////////////////////////////////////////////////////////////////

template <bool IsCopyable, bool IsMovable>
class enable_copy_move;

template <>
class enable_copy_move<true, true> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(const enable_copy_move &) = default;
	enable_copy_move(enable_copy_move &&) = default;

	RUA_OVERLOAD_ASSIGNMENT(enable_copy_move)
};

template <>
class enable_copy_move<true, false> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(const enable_copy_move &) = default;

	RUA_OVERLOAD_ASSIGNMENT_L(enable_copy_move)
};

template <>
class enable_copy_move<false, true> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(enable_copy_move &&) = default;

	RUA_OVERLOAD_ASSIGNMENT_R(enable_copy_move)
};

template <>
class enable_copy_move<false, false> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(const enable_copy_move &) = delete;
	enable_copy_move(enable_copy_move &&) = delete;

	enable_copy_move &operator=(const enable_copy_move &) = delete;
	enable_copy_move &operator=(enable_copy_move &&) = delete;
};

template <typename T>
using enable_copy_move_like = enable_copy_move<
	std::is_copy_constructible<T>::value && !std::is_const<T>::value,
	std::is_move_constructible<T>::value && !std::is_const<T>::value>;

////////////////////////////////////////////////////////////////////////////

#if defined(__cpp_lib_variant) || defined(__cpp_lib_any)

template <typename T>
using in_place_type_t = std::in_place_type_t<T>;

template <size_t I>
using in_place_index_t = std::in_place_index_t<I>;

#else

template <typename T>
struct in_place_type_t {};

template <size_t I>
struct in_place_index_t {};

#endif

////////////////////////////////////////////////////////////////////////////

template <typename T>
inline constexpr enable_if_t<std::is_convertible<T &&, bool>::value, bool>
is_valid(T &&val) {
	return static_cast<bool>(std::forward<T>(val));
}

template <typename T>
inline constexpr enable_if_t<!std::is_convertible<T &&, bool>::value, bool>
is_valid(T &&) {
	return true;
}

} // namespace rua

#endif
