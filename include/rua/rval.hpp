#ifndef _RUA_RVAL_HPP
#define _RUA_RVAL_HPP

#include "macros.hpp"
#include "type_traits/std_patch.hpp"

#include <utility>

namespace rua {

template <typename T>
class rval {
public:
	rval() {
		new (&_val) T();
	}

	template <
		typename... Args,
		typename ArgsFront = decay_t<argments_front_t<Args...>>,
		typename = enable_if_t<
			std::is_constructible<T, Args...>::value &&
			(sizeof...(Args) > 1 ||
			 (!std::is_base_of<T, ArgsFront>::value &&
			  !std::is_base_of<rval, ArgsFront>::value))>>
	rval(Args &&... args) {
		new (&_val) T(std::forward<Args>(args)...);
	}

	rval(T &&val) {
		new (&_val) T(std::move(val));
	}

	rval(T &val) : rval(std::move(val)) {}

	rval(const T &val) : rval(static_cast<T &&>(const_cast<T &>(val))) {}

	~rval() {
		_val.~T();
	}

	rval(rval &&src) {
		new (&_val) T(std::move(src._val));
	}

	rval(const rval &src) : rval(std::move(const_cast<rval &>(src))) {}

	RUA_OVERLOAD_ASSIGNMENT(rval)

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
