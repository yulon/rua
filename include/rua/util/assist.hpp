#ifndef _RUA_UTIL_ASSIST_HPP
#define _RUA_UTIL_ASSIST_HPP

#include "base.hpp"
#include "type_traits.hpp"

namespace rua {

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

////////////////////////////////////////////////////////////////////////////

#if RUA_CPP >= RUA_CPP_17
#define RUA_WEAK_FROM_THIS weak_from_this()
#else
#define RUA_WEAK_FROM_THIS shared_from_this()
#endif

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

template <typename L, typename R>
inline constexpr decltype(std::declval<L &&>() == std::declval<R &&>())
equal(L &&l, R &&r) {
	return std::forward<L>(l) == std::forward<R>(r);
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

template <typename T, typename... Args>
inline enable_if_t<std::is_trivial<T>::value && std::is_class<T>::value, T &>
construct(T &ref, Args &&...args) {
	return assign(ref, T(std::forward<Args>(args)...));
}

template <typename T, typename Arg>
inline enable_if_t<
	std::is_trivial<T>::value && !std::is_class<T>::value &&
		!std::is_array<T>::value && !std::is_function<T>::value,
	T &>
construct(T &ref, Arg &&arg) {
	return assign(ref, std::forward<Arg>(arg));
}

template <typename T, typename... Args>
inline enable_if_t<!std::is_trivial<T>::value || std::is_array<T>::value, T &>
construct(T &ref, Args &&...args) {
	return *(new (&ref) T(std::forward<Args>(args)...));
}

template <typename T, typename U, typename... Args>
inline enable_if_t<!std::is_trivial<T>::value, T &>
construct(T &ref, std::initializer_list<U> il, Args &&...args) {
	return *(new (&ref) T(il, std::forward<Args>(args)...));
}

template <typename T>
inline enable_if_t<std::is_trivially_destructible<T>::value> destruct(T &) {}

template <typename T>
inline enable_if_t<!std::is_trivially_destructible<T>::value> destruct(T &ref) {
	ref.~T();
}

////////////////////////////////////////////////////////////////////////////

#define RUA_OVERLOAD_ASSIGNMENT(T)                                             \
	template <typename From>                                                   \
	T &operator=(From &&from) {                                                \
		if (reinterpret_cast<uintptr_t>(this) ==                               \
			reinterpret_cast<uintptr_t>(&from)) {                              \
			return *this;                                                      \
		}                                                                      \
		destruct(*this);                                                       \
		construct(*this, std::forward<From>(from));                            \
		return *this;                                                          \
	}

#define RUA_OVERLOAD_ASSIGNMENT_S(T)                                           \
	template <typename From>                                                   \
	T &operator=(From &&from) {                                                \
		if (reinterpret_cast<uintptr_t>(this) ==                               \
			reinterpret_cast<uintptr_t>(&from)) {                              \
			return *this;                                                      \
		}                                                                      \
		T new_val(std::forward<From>(from));                                   \
		destruct(*this);                                                       \
		construct(*this, std::move(new_val));                                  \
		return *this;                                                          \
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

	RUA_OVERLOAD_ASSIGNMENT(enable_copy_move)
};

template <>
class enable_copy_move<false, true> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(enable_copy_move &&) = default;

	RUA_OVERLOAD_ASSIGNMENT(enable_copy_move)
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
inline Signed to_signed(Unsigned u) {
	return static_cast<Signed>(u);
}

template <typename Signed, typename Unsigned = make_unsigned_t<Signed>>
inline Unsigned to_unsigned(Signed i) {
	return static_cast<Unsigned>(i);
}

////////////////////////////////////////////////////////////////////////////

inline bool or_result() {
	return false;
}

template <typename FisrtCond, typename... Cond>
inline bool or_result(FisrtCond &&fisrt_cond, Cond &&...cond) {
	return static_cast<bool>(fisrt_cond) ||
		   or_result(std::forward<Cond>(cond)...);
}

} // namespace rua

#endif
