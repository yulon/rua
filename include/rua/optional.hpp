#ifndef _RUA_OPTIONAL_HPP
#define _RUA_OPTIONAL_HPP

#include "macros.hpp"
#include "types/traits.hpp"
#include "types/util.hpp"

#ifdef __cpp_lib_optional

#include <optional>

namespace rua {

using nullopt_t = std::nullopt_t;

RUA_INLINE_CONST auto nullopt = std::nullopt;

template <typename T>
class optional : public std::optional<T> {
public:
	constexpr optional(nullopt_t = nullopt) : std::optional<T>(nullopt) {}

	template <
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, Args &&...>::value &&
			(sizeof...(Args) > 1 ||
			 !std::is_base_of<
				 std::optional<T>,
				 decay_t<argments_front_t<Args...>>>::value)>>
	constexpr optional(Args &&... args) :
		std::optional<T>(std::in_place, std::forward<Args>(args)...) {}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args &&...>::
				value>>
	constexpr optional(std::initializer_list<U> il, Args &&... args) :
		std::optional<T>(std::in_place, il, std::forward<Args>(args)...) {}

	template <
		typename U,
		typename = enable_if_t<
			std::is_constructible<T, const U &>::value &&
			!std::is_constructible<T, optional<U> &&>::value>>
	optional(const std::optional<U> &src) : std::optional<T>(src) {}

	template <
		typename U,
		typename = enable_if_t<
			std::is_constructible<T, U &&>::value &&
			!std::is_constructible<T, optional<U> &&>::value>>
	optional(std::optional<U> &&src) : std::optional<T>(std::move(src)) {}

	template <class... Args>
	invoke_result_t<T &, Args &&...> operator()(Args &&... args) & {
		return this->value()(std::forward<Args>(args)...);
	}

	template <class... Args>
	invoke_result_t<const T &, Args &&...> operator()(Args &&... args) const & {
		return this->value()(std::forward<Args>(args)...);
	}

	template <class... Args>
	invoke_result_t<T &&, Args &&...> operator()(Args &&... args) && {
		return std::move(*this).value()(std::forward<Args>(args)...);
	}
};

} // namespace rua

#else

namespace rua {

struct nullopt_t {};

RUA_INLINE_CONST nullopt_t nullopt;

template <typename T>
class _optional_base {
public:
	using value_type = T;

	constexpr _optional_base(bool has_val = false) :
		_sto(), _has_val(has_val) {}

	~_optional_base() {
		reset();
	}

	_optional_base(const _optional_base &src) : _has_val(src._has_val) {
		if (!src._has_val) {
			return;
		}
		new (&value()) T(src.value());
	}

	_optional_base(_optional_base &&src) : _has_val(src._has_val) {
		if (!src._has_val) {
			return;
		}
		new (&value()) T(std::move(src.value()));
		src.value().~T();
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

	template <
		typename U,
		typename = enable_if_t<
			std::is_copy_constructible<T>::value &&
			std::is_convertible<U, T>::value>>
	T value_or(U &&default_value) const & {
		return _has_val ? value()
						: static_cast<T>(std::forward<U>(default_value));
	}

	template <
		typename U,
		typename = enable_if_t<
			std::is_move_constructible<T>::value &&
			std::is_convertible<U, T>::value>>
	T value_or(U &&default_value) && {
		return _has_val ? std::move(value())
						: static_cast<T>(std::forward<U>(default_value));
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

	void reset() {
		if (!_has_val) {
			return;
		}
		value().~T();
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
	alignas(alignof(T)) byte _sto[sizeof(T)];
	bool _has_val;

	template <
		typename... Args,
		typename = enable_if_t<std::is_constructible<T, Args...>::value>>
	void _emplace(Args &&... args) {
		new (&value()) T(std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args...>::value>>
	void _emplace(std::initializer_list<U> il, Args &&... args) {
		new (&value()) T(il, std::forward<Args>(args)...);
	}
};

template <typename T>
class optional : public _optional_base<T>, private enable_copy_move_like<T> {
public:
	constexpr optional(nullopt_t = nullopt) : _optional_base<T>(false) {}

	template <
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, Args...>::value &&
			(sizeof...(Args) > 1 ||
			 !std::is_base_of<optional, decay_t<argments_front_t<Args...>>>::
				 value)>>
	optional(Args &&... args) : _optional_base<T>(true) {
		this->_emplace(std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args...>::value>>
	optional(std::initializer_list<U> il, Args &&... args) :
		_optional_base<T>(true) {
		this->_emplace(il, std::forward<Args>(args)...);
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
};

} // namespace rua

#endif

#endif
