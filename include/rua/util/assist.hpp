#ifndef _RUA_UTIL_ASSIST_HPP
#define _RUA_UTIL_ASSIST_HPP

#include "base.hpp"
#include "type_traits.hpp"

#include "../invocable.hpp"

namespace rua {

#define RUA_CONTAINER_OF(_member_ptr, _type, _member)                          \
	reinterpret_cast<type *>(                                                  \
		reinterpret_cast<uintptr_t>(_member_ptr) - offsetof(_type, _member))

#define RUA_IS_BASE_OF_CONCEPT(_B, _D)                                         \
	typename _D,                                                               \
		typename =                                                             \
			enable_if_t<std::is_base_of<_B, remove_reference_t<_D>>::value>

#define RUA_DERIVED_CONCEPT(_B, _D)                                            \
	typename _D,                                                               \
		typename = enable_if_t <                                               \
					   std::is_base_of<_B, remove_reference_t<_D>>::value &&   \
				   !std::is_same<_B, _D>::value >

#define RUA_CONSTRUCTIBLE_CONCEPT(_Args, _Constructible, _Exclusion)           \
	template <                                                                 \
		typename... _Args,                                                     \
		typename ArgsFront = decay_t<front_t<_Args &&...>>,                    \
		typename = enable_if_t<                                                \
			(sizeof...(_Args) > 1 &&                                           \
			 std::is_constructible<_Constructible, _Args &&...>::value) ||     \
			(!std::is_base_of<_Exclusion, ArgsFront>::value &&                 \
			 std::is_convertible<ArgsFront, _Constructible>::value)>>

#define RUA_ARG(...) __VA_ARGS__

////////////////////////////////////////////////////////////////////////////

#if RUA_CPP >= RUA_CPP_17
#define RUA_WEAK_FROM_THIS weak_from_this()
#else
#define RUA_WEAK_FROM_THIS shared_from_this()
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

RUA_CVAL auto nullpos = nmax<size_t>();

#define RUA_FULLPTR(T) (reinterpret_cast<T *>(nmax<uintptr_t>()))

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
class enable_copy_move<true, true> {};

template <>
class enable_copy_move<true, false> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(const enable_copy_move &) = default;

	enable_copy_move &operator=(const enable_copy_move &) = default;
};

template <>
class enable_copy_move<false, true> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(enable_copy_move &&) = default;

	enable_copy_move &operator=(enable_copy_move &&) = default;
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

template <typename Valuer, typename Value>
class enable_value_operators {
public:
	Value &operator*() & {
		return _this()->value();
	}

	const Value &operator*() const & {
		return _this()->value();
	}

	Value &&operator*() && {
		return std::move(_this()->value());
	}

	Value *operator->() {
		return &_this()->value();
	}

	const Value *operator->() const {
		return &_this()->value();
	}

	template <class... Args>
	invoke_result_t<Value &, Args &&...> operator()(Args &&...args) & {
		return _this()->value()(std::forward<Args>(args)...);
	}

	template <class... Args>
	invoke_result_t<const Value &, Args &&...>
	operator()(Args &&...args) const & {
		return _this()->value()(std::forward<Args>(args)...);
	}

	template <class... Args>
	invoke_result_t<Value &&, Args &&...> operator()(Args &&...args) && {
		return std::move(_this()->value())(std::forward<Args>(args)...);
	}

	template <
		typename DefaultValue,
		typename = enable_if_t<
			std::is_copy_constructible<Value>::value &&
			std::is_convertible<DefaultValue &&, Value>::value>>
	Value value_or(DefaultValue &&default_value) const & {
		return _this()->has_value()
				   ? _this()->value()
				   : static_cast<Value>(
						 std::forward<DefaultValue>(default_value));
	}

	template <
		typename DefaultValue,
		typename = enable_if_t<
			std::is_move_constructible<Value>::value &&
			std::is_convertible<DefaultValue &&, Value>::value>>
	Value value_or(DefaultValue &&default_value) && {
		return _this()->has_value()
				   ? std::move(_this()->value())
				   : static_cast<Value>(
						 std::forward<DefaultValue>(default_value));
	}

protected:
	enable_value_operators() = default;

private:
	Valuer *_this() {
		return static_cast<Valuer *>(this);
	}

	const Valuer *_this() const {
		return static_cast<const Valuer *>(this);
	}
};

template <typename Valuer>
class enable_value_operators<Valuer, void> {
public:
	RUA_CONSTEXPR_14 void operator*() const {}

protected:
	enable_value_operators() = default;

private:
	Valuer *_this() {
		return static_cast<Valuer *>(this);
	}

	const Valuer *_this() const {
		return static_cast<const Valuer *>(this);
	}
};

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

template <typename R = bool>
inline R or_result() {
	return R();
}

template <typename FisrtR, typename... R>
inline FisrtR or_result(FisrtR &&fisrt_r, R &&...r) {
	if (static_cast<bool>(fisrt_r)) {
		return fisrt_r;
	}
	return or_result<FisrtR>(std::forward<R>(r)...);
}

////////////////////////////////////////////////////////////////////////////

template <typename... Args>
static inline void no_effect(Args &&...) {}

} // namespace rua

#endif
