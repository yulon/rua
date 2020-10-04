#ifndef _RUA_MOVE_ONLY_HPP
#define _RUA_MOVE_ONLY_HPP

#include "macros.hpp"
#include "types/traits.hpp"
#include "types/util.hpp"

namespace rua {

template <typename T>
class move_only {
public:
	move_only() {
		new (&value()) T();
	}

	template <
		typename... Args,
		typename ArgsFront = decay_t<argments_front_t<Args...>>,
		typename = enable_if_t<
			std::is_constructible<T, Args &&...>::value &&
			(sizeof...(Args) > 1 ||
			 (!std::is_base_of<T, ArgsFront>::value &&
			  !std::is_base_of<move_only, ArgsFront>::value))>>
	move_only(Args &&... args) {
		new (&value()) T(std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename... Args,
		typename ArgsFront = decay_t<argments_front_t<Args...>>,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args &&...>::
				value>>
	move_only(std::initializer_list<U> il, Args &&... args) {
		new (&value()) T(il, std::forward<Args>(args)...);
	}

	move_only(T &&val) {
		new (&value()) T(std::move(val));
	}

	move_only(T &val) : move_only(std::move(val)) {}

	move_only(const T &val) :
		move_only(static_cast<T &&>(const_cast<T &>(val))) {}

	~move_only() {
		value().~T();
	}

	move_only(move_only &&src) {
		new (&value()) T(std::move(src.value()));
	}

	move_only(const move_only &src) :
		move_only(std::move(const_cast<move_only &>(src))) {}

	RUA_OVERLOAD_ASSIGNMENT(move_only)

	T &value() & {
		return *reinterpret_cast<T *>(&_sto[0]);
	}

	const T &value() const & {
		return *reinterpret_cast<const T *>(&_sto[0]);
	}

	T &&value() && {
		return std::move(*reinterpret_cast<T *>(&_sto[0]));
	}

	T &operator*() & {
		return value();
	}

	const T &operator*() const & {
		return value();
	}

	T &&operator*() && {
		return std::move(value());
	}

	operator T &() & {
		return value();
	}

	operator const T &() const & {
		return value();
	}

	operator T &&() && {
		return std::move(value());
	}

	T *operator->() {
		return &value();
	}

	const T *operator->() const {
		return &value();
	}

	template <class... Args>
	invoke_result_t<T &, Args &&...> operator()(Args &&... args) & {
		return value()(std::forward<Args>(args)...);
	}

	template <class... Args>
	invoke_result_t<const T &, Args &&...> operator()(Args &&... args) const & {
		return value()(std::forward<Args>(args)...);
	}

	template <class... Args>
	invoke_result_t<T &&, Args &&...> operator()(Args &&... args) && {
		return std::move(value())(std::forward<Args>(args)...);
	}

private:
	alignas(alignof(T)) byte _sto[sizeof(T)];
};

} // namespace rua

#endif
