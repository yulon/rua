#ifndef _rua_optional_hpp
#define _rua_optional_hpp

#include "invocable.hpp"
#include "util.hpp"

#ifdef __cpp_lib_optional

#include <optional>

namespace rua {

using nullopt_t = std::nullopt_t;

RUA_CVAL auto nullopt = std::nullopt;

template <typename T>
class optional : public std::optional<T> {
public:
	constexpr optional(nullopt_t = nullopt) : std::optional<T>(nullopt) {}

	template <
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, Args &&...>::value &&
			(sizeof...(Args) > 1 ||
			 !std::is_base_of<std::optional<T>, decay_t<front_t<Args...>>>::
				 value)>>
	constexpr optional(Args &&...args) :
		std::optional<T>(std::in_place, std::forward<Args>(args)...) {}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args &&...>::
				value>>
	constexpr optional(std::initializer_list<U> il, Args &&...args) :
		std::optional<T>(std::in_place, il, std::forward<Args>(args)...) {}

	template <
		typename U,
		typename = enable_if_t<
			std::is_constructible<T, const U &>::value &&
			!std::is_constructible<T, const optional<U> &>::value>>
	optional(const optional<U> &src) : std::optional<T>(src) {}

	template <
		typename U,
		typename = enable_if_t<
			std::is_constructible<T, U &&>::value &&
			!std::is_constructible<T, optional<U> &&>::value>>
	optional(optional<U> &&src) : std::optional<T>(std::move(src)) {}

	template <class... Args>
	invoke_result_t<T &, Args &&...> operator()(Args &&...args) & {
		return this->value()(std::forward<Args>(args)...);
	}

	template <class... Args>
	invoke_result_t<const T &, Args &&...> operator()(Args &&...args) const & {
		return this->value()(std::forward<Args>(args)...);
	}

	template <class... Args>
	invoke_result_t<T &&, Args &&...> operator()(Args &&...args) && {
		return std::move(*this).value()(std::forward<Args>(args)...);
	}
};

} // namespace rua

#else

namespace rua {

struct nullopt_t {};

RUA_CVAL nullopt_t nullopt;

template <typename T>
class $optional_base {
public:
	using value_type = T;

	constexpr $optional_base(bool has_val = false) :
		$sto(), $has_val(has_val) {}

	~$optional_base() {
		reset();
	}

	$optional_base(const $optional_base &src) : $has_val(src.$has_val) {
		if (!src.$has_val) {
			return;
		}
		$emplace(src.value());
	}

	$optional_base($optional_base &&src) : $has_val(src.$has_val) {
		if (!src.$has_val) {
			return;
		}
		$emplace(std::move(src.value()));
		src.value().~T();
		src.$has_val = false;
	}

	RUA_OVERLOAD_ASSIGNMENT($optional_base)

	bool has_value() const {
		return $has_val;
	}

	explicit operator bool() const {
		return $has_val;
	}

	T &value() & {
		return *reinterpret_cast<T *>(&$sto[0]);
	}

	const T &value() const & {
		return *reinterpret_cast<const T *>(&$sto[0]);
	}

	T &&value() && {
		return std::move(*reinterpret_cast<T *>(&$sto[0]));
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
			std::is_convertible<U &&, T>::value>>
	T value_or(U &&default_value) const & {
		return $has_val ? value()
						: static_cast<T>(std::forward<U>(default_value));
	}

	template <
		typename U,
		typename = enable_if_t<
			std::is_move_constructible<T>::value &&
			std::is_convertible<U &&, T>::value>>
	T value_or(U &&default_value) && {
		return $has_val ? std::move(value())
						: static_cast<T>(std::forward<U>(default_value));
	}

	T *operator->() {
		return &value();
	}

	const T *operator->() const {
		return &value();
	}

	template <class... Args>
	invoke_result_t<T &, Args &&...> operator()(Args &&...args) & {
		return value()(std::forward<Args>(args)...);
	}

	template <class... Args>
	invoke_result_t<const T &, Args &&...> operator()(Args &&...args) const & {
		return value()(std::forward<Args>(args)...);
	}

	template <class... Args>
	invoke_result_t<T &&, Args &&...> operator()(Args &&...args) && {
		return std::move(value())(std::forward<Args>(args)...);
	}

	void reset() {
		if (!$has_val) {
			return;
		}
		value().~T();
		$has_val = false;
	}

	template <
		typename... Args,
		typename = enable_if_t<std::is_constructible<T, Args...>::value>>
	void emplace(Args &&...args) {
		reset();
		$emplace(std::forward<Args>(args)...);
		$has_val = true;
	}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args...>::value>>
	void emplace(std::initializer_list<U> il, Args &&...args) {
		reset();
		$emplace(il, std::forward<Args>(args)...);
		$has_val = true;
	}

protected:
	alignas(alignof(T)) uchar $sto[sizeof(T)];
	bool $has_val;

	template <
		typename... Args,
		typename = enable_if_t<std::is_constructible<T, Args...>::value>>
	void $emplace(Args &&...args) {
		construct(value(), std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args...>::value>>
	void $emplace(std::initializer_list<U> il, Args &&...args) {
		construct(value(), il, std::forward<Args>(args)...);
	}
};

template <typename T>
class optional : public $optional_base<T>, private enable_copy_move_like<T> {
public:
	constexpr optional(nullopt_t = nullopt) : $optional_base<T>(false) {}

	template <
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, Args...>::value &&
			(sizeof...(Args) > 1 ||
			 !std::is_base_of<optional, decay_t<front_t<Args...>>>::value)>>
	optional(Args &&...args) : $optional_base<T>(true) {
		this->$emplace(std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args...>::value>>
	optional(std::initializer_list<U> il, Args &&...args) :
		$optional_base<T>(true) {
		this->$emplace(il, std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename = enable_if_t<
			std::is_constructible<T, const U &>::value &&
			!std::is_constructible<T, const optional<U> &>::value>>
	optional(const optional<U> &src) : $optional_base<T>(src.has_value()) {
		if (!src.has_value()) {
			return;
		}
		this->$emplace(src.value());
	}

	template <
		typename U,
		typename = enable_if_t<
			std::is_constructible<T, U &&>::value &&
			!std::is_constructible<T, optional<U> &&>::value>>
	optional(optional<U> &&src) : $optional_base<T>(src.has_value()) {
		if (!src.has_value()) {
			return;
		}
		this->$emplace(std::move(src).value());
	}
};

} // namespace rua

#endif

#endif
