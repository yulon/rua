#ifndef _RUA_TYPES_UTIL_HPP
#define _RUA_TYPES_UTIL_HPP

#include "../macros.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <type_traits>
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

namespace rua {

using uint = unsigned int;
using ushort = unsigned short;
using schar = signed char;
using uchar = unsigned char;

////////////////////////////////////////////////////////////////////////////

/*
	Q: Why don't I use std::byte?
	A: Because I suspect std::byte is used to disgusting Microsoft.

	1. All operators are implemented by operator overloading,
	   but MSVC does not inline code in debug mode,
	   which causes programs in debug mode to run slowly.

	2. List initialization declarations are very redundant,
	   such as {std::byte{n}, ...} or {static_cast<std::byte>(n), ...},
	   but G++ can use {{n}, ...}.
*/
using byte = uchar;

#if defined(__cpp_lib_byte)
using std_byte = std::byte;
#else
using std_byte = uchar;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T>
inline constexpr T nmax() {
	return (std::numeric_limits<T>::max)();
}

template <typename T>
inline constexpr T nmin() {
	return (std::numeric_limits<T>::min)();
}

template <typename T>
inline constexpr T nlowest() {
	return (std::numeric_limits<T>::lowest)();
}

////////////////////////////////////////////////////////////////////////////

template <typename A, typename B>
inline decltype(std::declval<A &&>() = std::declval<B &&>())
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

#if defined(__cpp_lib_optional) || defined(__cpp_lib_variant) ||               \
	defined(__cpp_lib_any)

using in_place_t = std::in_place_t;

template <typename T>
using in_place_type_t = std::in_place_type_t<T>;

template <size_t I>
using in_place_index_t = std::in_place_index_t<I>;

#else

struct in_place_t {};

template <typename T>
struct in_place_type_t {};

template <size_t I>
struct in_place_index_t {};

#endif

RUA_INLINE_CONST in_place_t in_place{};

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct size_of {
	static constexpr size_t value = sizeof(T);
};

template <>
struct size_of<void> {
	static constexpr size_t value = 0;
};

template <typename R, typename... Args>
struct size_of<R(Args...)> {
	static constexpr size_t value = 0;
};

template <typename T>
struct size_of<T &> {
	static constexpr size_t value = 0;
};

template <typename T>
struct size_of<T &&> {
	static constexpr size_t value = 0;
};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto size_of_v = size_of<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct align_of {
	static constexpr size_t value = alignof(T);
};

template <>
struct align_of<void> {
	static constexpr size_t value = 0;
};

template <typename R, typename... Args>
struct align_of<R(Args...)> {
	static constexpr size_t value = 0;
};

template <typename T>
struct align_of<T &> {
	static constexpr size_t value = 0;
};

template <typename T>
struct align_of<T &&> {
	static constexpr size_t value = 0;
};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto align_of_v = align_of<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename... Types>
struct max_size_of;

template <typename Last>
struct max_size_of<Last> {
	static constexpr size_t value = size_of<Last>::value;
};

template <typename First, typename... Others>
struct max_size_of<First, Others...> {
	static constexpr size_t value =
		size_of<First>::value > max_size_of<Others...>::value
			? size_of<First>::value
			: max_size_of<Others...>::value;
};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename... Types>
inline constexpr auto max_size_of_v = max_size_of<Types...>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename... Types>
struct max_align_of;

template <typename Last>
struct max_align_of<Last> {
	static constexpr size_t value = align_of<Last>::value;
};

template <typename First, typename... Others>
struct max_align_of<First, Others...> {
	static constexpr size_t value =
		align_of<First>::value > max_align_of<Others...>::value
			? align_of<First>::value
			: max_align_of<Others...>::value;
};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename... Types>
inline constexpr auto max_align_of_v = max_align_of<Types...>::value;
#endif

////////////////////////////////////////////////////////////////////////////

RUA_INLINE_CONST auto nullpos = nmax<size_t>();

template <size_t C, typename T, typename... Types>
struct _index_of;

template <size_t C, typename T>
struct _index_of<C, T> {
	static constexpr size_t value = nullpos;
};

template <size_t C, typename T, typename Cur, typename... Others>
struct _index_of<C, T, Cur, Others...> {
	static constexpr size_t value = std::is_same<T, Cur>::value
										? C - sizeof...(Others) - 1
										: _index_of<C, T, Others...>::value;
};

template <typename T, typename... Types>
struct index_of : _index_of<sizeof...(Types), T, Types...> {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T, typename... Types>
inline constexpr auto index_of_v = index_of<T, Types...>::value;
#endif

} // namespace rua

#endif
