#ifndef _RUA_OPTIONAL_HPP
#define _RUA_OPTIONAL_HPP

#include "macros.hpp"
#include "type_traits/std_patch.hpp"

#ifdef __cpp_lib_optional

#include <optional>

namespace rua {

template <typename T>
using optional = std::optional<T>;

using nullopt_t = std::nullopt_t;

RUA_MULTIDEF_VAR constexpr auto nullopt = std::nullopt;

} // namespace rua

#else

#include "type_traits/enable_copy_move.hpp"

#include <cstdint>
#include <initializer_list>
#include <utility>

namespace rua {

struct nullopt_t {};

RUA_MULTIDEF_VAR constexpr nullopt_t nullopt;

template <typename T>
class _optional_base {
public:
	using value_type = T;

	constexpr _optional_base() = default;

	constexpr _optional_base(bool has_val) : _empty(), _has_val(has_val) {}

	~_optional_base() {
		reset();
	}

	_optional_base(const _optional_base &src) : _has_val(src._has_val) {
		if (!src._has_val) {
			return;
		}
		new (&_val) T(src._val);
	}

	_optional_base(_optional_base &&src) : _has_val(src._has_val) {
		if (!src._has_val) {
			return;
		}
		new (&_val) T(std::move(src._val));
		src._val.~T();
		src._has_val = false;
	}

	RUA_OVERLOAD_ASSIGNMENT(_optional_base)

	bool has_value() const {
		return _has_val;
	}

	explicit operator bool() const {
		return _has_val;
	}

	T &value() & {
		return _val;
	}

	const T &value() const & {
		return _val;
	}

	T &&value() && {
		return std::move(_val);
	}

	T &operator*() & {
		return _val;
	}

	const T &operator*() const & {
		return _val;
	}

	T &&operator*() && {
		return std::move(_val);
	}

	template <
		typename U,
		typename = enable_if_t<
			std::is_copy_constructible<T>::value &&
			std::is_convertible<U, T>::value>>
	T value_or(U &&default_value) const & {
		return _has_val ? _val : static_cast<T>(std::forward<U>(default_value));
	}

	template <
		typename U,
		typename = enable_if_t<
			std::is_move_constructible<T>::value &&
			std::is_convertible<U, T>::value>>
	T value_or(U &&default_value) && {
		return _has_val ? std::move(_val)
						: static_cast<T>(std::forward<U>(default_value));
	}

	T *operator->() {
		return &_val;
	}

	const T *operator->() const {
		return &_val;
	}

	void reset() {
		if (!_has_val) {
			return;
		}
		_val.~T();
		_has_val = false;
	}

	template <
		typename... Args,
		typename = enable_if_t<std::is_constructible<T, Args...>::value>>
	void emplace(Args &&... args) {
		reset();
		_emplace(std::forward<Args>(args)...);
		_has_val = true;
	}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args...>::value>>
	void emplace(std::initializer_list<U> il, Args &&... args) {
		reset();
		_emplace(il, std::forward<Args>(args)...);
		_has_val = true;
	}

protected:
	struct _empty_t {};
	union {
		_empty_t _empty;
		decay_t<T> _val;
	};
	bool _has_val;

	template <
		typename... Args,
		typename = enable_if_t<std::is_constructible<T, Args...>::value>>
	void _emplace(Args &&... args) {
		new (&_val) T(std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args...>::value>>
	void _emplace(std::initializer_list<U> il, Args &&... args) {
		new (&_val) T(il, std::forward<Args>(args)...);
	}
};

template <typename T>
class optional : public _optional_base<T>, private enable_copy_move_like<T> {
public:
	constexpr optional() : _optional_base<T>(false) {}

	constexpr optional(nullopt_t) : _optional_base<T>(false) {}

	template <
		typename U = T,
		typename = enable_if_t<
			std::is_constructible<T, U &&>::value &&
			!std::is_base_of<optional<T>, decay_t<U>>::value>>
	optional(U &&val) : _optional_base<T>(true) {
		this->_emplace(std::forward<U>(val));
	}

	template <
		typename U,
		typename = enable_if_t<
			std::is_constructible<T, const U &>::value &&
			!std::is_constructible<T, optional<U> &&>::value>>
	optional(const optional<U> &src) : _optional_base<T>(src.has_value()) {
		if (!src.has_value()) {
			return;
		}
		this->_emplace(src.value());
	}

	template <
		typename U,
		typename = enable_if_t<
			std::is_constructible<T, U &&>::value &&
			!std::is_constructible<T, optional<U> &&>::value>>
	optional(optional<U> &&src) : _optional_base<T>(src.has_value()) {
		if (!src.has_value()) {
			return;
		}
		this->_emplace(std::move(src).value());
	}

	template <
		typename... Args,
		typename = enable_if_t<std::is_constructible<T, Args...>::value>>
	explicit optional(in_place_t, Args &&... args) : _optional_base<T>(true) {
		this->_emplace(std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args...>::value>>
	explicit optional(
		in_place_t, std::initializer_list<U> il, Args &&... args) :
		_optional_base<T>(true) {
		this->_emplace(il, std::forward<Args>(args)...);
	}
};

} // namespace rua

#endif

#include <initializer_list>
#include <utility>

namespace rua {

template <typename T>
optional<decay_t<T>> make_optional(T &&val) {
	return optional<decay_t<T>>(std::forward<T>(val));
}

template <typename T, typename... Args>
optional<T> make_optional(Args &&... args) {
	return optional<T>(in_place, std::forward<Args>(args)...);
}

template <typename T, typename U, typename... Args>
optional<T> make_optional(std::initializer_list<U> il, Args &&... args) {
	return optional<T>(in_place, il, std::forward<Args>(args)...);
}

} // namespace rua

#endif
