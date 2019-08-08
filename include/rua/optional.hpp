#ifndef _RUA_OPTIONAL_HPP
#define _RUA_OPTIONAL_HPP

#include "in_place.hpp"
#include "macros.hpp"

#if RUA_CPP >= RUA_CPP_17 || defined(__cpp_lib_optional)

#include <optional>

namespace rua {

template <typename T>
using optional = std::optional<T>;

using nullopt_t = std::nullopt_t;

RUA_MULTIDEF_VAR constexpr auto nullopt = std::nullopt;

} // namespace rua

#else

#include "type_traits.hpp"

#include <array>
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

	_optional_base() = default;

	constexpr _optional_base(bool has_val) : _val(), _has_val(has_val) {}

	~_optional_base() {
		reset();
	}

	_optional_base(const _optional_base &src) : _has_val(src._has_val) {
		if (!src._has_val) {
			return;
		}
		src._has_val = false;
		new (_val_ptr()) T(*src._val_ptr());
	}

	_optional_base(_optional_base &&src) : _has_val(src._has_val) {
		if (!src._has_val) {
			return;
		}
		src._has_val = false;
		new (_val_ptr()) T(std::move(*src._val_ptr()));
	}

	RUA_OVERLOAD_ASSIGNMENT(_optional_base)

	bool has_value() const {
		return _has_val;
	}

	explicit operator bool() const {
		return _has_val;
	}

	T &value() & {
		return *_val_ptr();
	}

	const T &value() const & {
		return *_val_ptr();
	}

	T &&value() && {
		return std::move(*_val_ptr());
	}

	T &operator*() & {
		return *_val_ptr();
	}

	const T &operator*() const & {
		return *_val_ptr();
	}

	T &&operator*() && {
		return std::move(*_val_ptr());
	}

	template <
		typename U,
		typename = typename std::enable_if<
			std::is_copy_constructible<T>::value &&
			std::is_convertible<U, T>::value>::type>
	T value_or(U &&default_value) const & {
		return _has_val ? *_val_ptr()
						: static_cast<T>(std::forward<U>(default_value));
	}

	template <
		typename U,
		typename = typename std::enable_if<
			std::is_move_constructible<T>::value &&
			std::is_convertible<U, T>::value>::type>
	T value_or(U &&default_value) && {
		return _has_val ? std::move(*_val_ptr())
						: static_cast<T>(std::forward<U>(default_value));
	}

	T *operator->() {
		return _val_ptr();
	}

	const T *operator->() const {
		return _val_ptr();
	}

	void reset() {
		if (!_has_val) {
			return;
		}
		_val_ptr()->~T();
		_has_val = false;
	}

	template <
		typename... Args,
		typename = typename std::enable_if<
			std::is_constructible<T, Args...>::value>::type>
	void emplace(Args &&... args) {
		reset();
		_emplace(std::forward<Args>(args)...);
		_has_val = true;
	}

	template <
		typename U,
		typename... Args,
		typename = typename std::enable_if<
			std::is_constructible<T, std::initializer_list<U>, Args...>::
				value>::type>
	void emplace(std::initializer_list<U> il, Args &&... args) {
		reset();
		_emplace(il, std::forward<Args>(args)...);
		_has_val = true;
	}

protected:
	std::array<uint8_t, sizeof(T)> _val;
	bool _has_val;

	RUA_FORCE_INLINE T *_val_ptr() {
		return reinterpret_cast<T *>(_val.data());
	}

	RUA_FORCE_INLINE const T *_val_ptr() const {
		return reinterpret_cast<const T *>(_val.data());
	}

	template <
		typename... Args,
		typename = typename std::enable_if<
			std::is_constructible<T, Args...>::value>::type>
	void _emplace(Args &&... args) {
		new (_val_ptr()) T(std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename... Args,
		typename = typename std::enable_if<
			std::is_constructible<T, std::initializer_list<U>, Args...>::
				value>::type>
	void _emplace(std::initializer_list<U> il, Args &&... args) {
		new (_val_ptr()) T(il, std::forward<Args>(args)...);
	}
};

template <typename T>
class optional : public _optional_base<T>, private enable_copy_move_from<T> {
public:
	constexpr optional() : _optional_base<T>(false) {}

	constexpr optional(nullopt_t) : _optional_base<T>(false) {}

	explicit optional(T &&val) : _optional_base<T>(true) {
		this->_emplace(std::move(val));
	}

	template <
		typename... Args,
		typename = typename std::enable_if<
			std::is_constructible<T, Args...>::value>::type>
	explicit optional(in_place_t, Args &&... args) : _optional_base<T>(true) {
		this->_emplace(std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename... Args,
		typename = typename std::enable_if<
			std::is_constructible<T, std::initializer_list<U>, Args...>::
				value>::type>
	explicit optional(
		in_place_t, std::initializer_list<U> il, Args &&... args) :
		_optional_base<T>(true) {
		this->_emplace(il, std::forward<Args>(args)...);
	}
};

} // namespace rua

#endif

#include <initializer_list>
#include <type_traits>
#include <utility>

namespace rua {

template <typename T>
optional<typename std::decay<T>::type> make_optional(T &&val) {
	return optional<typename std::decay<T>::type>(std::forward<T>(val));
}

template <class T, class... Args>
optional<T> make_optional(Args &&... args) {
	return optional<T>(in_place, std::forward<Args>(args)...);
}

template <class T, class U, class... Args>
optional<T> make_optional(std::initializer_list<U> il, Args &&... args) {
	return optional<T>(in_place, il, std::forward<Args>(args)...);
}

} // namespace rua

#endif
