#ifndef _RUA_TYPES_UTIL_HPP
#define _RUA_TYPES_UTIL_HPP

#include "def.hpp"
#include "traits.hpp"

#include "../macros.hpp"

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
			return *this;                                                      \
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

#define RUA_OVERLOAD_ASSIGNMENT_S(T)                                           \
	T &operator=(const T &src) {                                               \
		if (this == &src) {                                                    \
			return *this;                                                      \
		}                                                                      \
		return operator=(T(src));                                              \
	}                                                                          \
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

template <typename T, typename U = T>
inline constexpr decltype(std::declval<T &&>() = std::declval<U &&>())
assign(T &&obj, U &&value) {
	return std::forward<T>(obj) = std::forward<U>(value);
}

template <typename T, typename U = decay_t<T>, typename R = decay_t<T>>
inline RUA_CONSTEXPR_14 R exchange(T &&obj, U &&new_value) {
	R old_value = std::move(obj);
	std::forward<T>(obj) = std::forward<U>(new_value);
	return old_value;
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

template <typename T>
inline constexpr enable_if_t<std::is_constructible<bool, T &&>::value, bool>
is_valid(T &&val) {
	return static_cast<bool>(std::forward<T>(val));
}

template <typename T>
inline constexpr enable_if_t<!std::is_constructible<bool, T &&>::value, bool>
is_valid(T &&) {
	return true;
}

template <typename T>
inline constexpr enable_if_t<std::is_constructible<bool, T &>::value, bool>
is_valid(T *val) {
	return val && static_cast<bool>(*val);
}

template <typename T>
inline constexpr enable_if_t<!std::is_constructible<bool, T &>::value, bool>
is_valid(T *val) {
	return val;
}

////////////////////////////////////////////////////////////////////////////

template <typename Unsigned, typename Signed = make_signed_t<Unsigned>>
Signed to_signed(Unsigned u) {
	return static_cast<Signed>(u);
}

template <typename Signed, typename Unsigned = make_unsigned_t<Signed>>
Unsigned to_unsigned(Signed i) {
	return static_cast<Unsigned>(i);
}

////////////////////////////////////////////////////////////////////////////

#ifdef __cpp_binary_literals

#define RUA_B(n) (0b##n)

#else

inline constexpr int _b_non_lowest(int n, int off);

inline constexpr int _b_non_zero(int n, int off) {
	return n ? _b_non_lowest(n, off) : 0;
}

inline constexpr int _b_non_lowest(int n, int off) {
	return (n & 1) << off | _b_non_zero(n / 10, off + 1);
}

inline constexpr int _b(int n) {
	return (n & 1) | _b_non_lowest(n / 10, 1);
}

#define RUA_B(n) (rua::_b(n))

#endif

} // namespace rua

#endif
