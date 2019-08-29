#ifndef _RUA_ALWAYS_MOVE_HPP
#define _RUA_ALWAYS_MOVE_HPP

#include "macros.hpp"
#include "type_traits.hpp"

#include <utility>

namespace rua {

template <typename T>
class always_move {
public:
	always_move() {
		new (&_val) T();
	}

	template <
		typename... Args,
		typename ArgsFront =
			typename std::decay<argments_front_t<Args...>>::type,
		typename = typename std::enable_if<
			std::is_constructible<T, Args...>::value &&
			(sizeof...(Args) > 1 ||
			 (!std::is_base_of<T, ArgsFront>::value &&
			  !std::is_base_of<always_move, ArgsFront>::value))>::type>
	always_move(Args &&... args) {
		new (&_val) T(std::forward<Args>(args)...);
	}

	always_move(T &&val) {
		new (&_val) T(std::move(val));
	}

	always_move(T &val) : always_move(std::move(val)) {}

	always_move(const T &val) :
		always_move(static_cast<T &&>(const_cast<T &>(val))) {}

	~always_move() {
		_val.~T();
	}

	always_move(always_move &&src) {
		new (&_val) T(std::move(src._val));
	}

	always_move(const always_move &src) :
		always_move(
			static_cast<always_move &&>(const_cast<always_move &>(src))) {}

	RUA_OVERLOAD_ASSIGNMENT(always_move)

	T &&value() && {
		return std::move(_val);
	}

	T &value() & {
		return _val;
	}

	const T &value() const & {
		return _val;
	}

	T &&operator*() && {
		return std::move(_val);
	}

	T &operator*() & {
		return _val;
	}

	const T &operator*() const & {
		return _val;
	}

	operator T &&() && {
		return std::move(_val);
	}

	operator T &() & {
		return _val;
	}

	operator const T &() const & {
		return _val;
	}

	T *operator->() {
		return &_val;
	}

	const T *operator->() const {
		return &_val;
	}

private:
	struct _empty_t {};
	union {
		_empty_t _empty;
		T _val;
	};
};

} // namespace rua

#endif
