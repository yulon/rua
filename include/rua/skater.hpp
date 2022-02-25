#ifndef _RUA_SKATER_HPP
#define _RUA_SKATER_HPP

#include "macros.hpp"
#include "types/traits.hpp"
#include "types/util.hpp"

namespace rua {

template <typename T>
class skater {
public:
	skater() {
		new (&value()) T();
	}

	template <
		typename... Args,
		typename ArgsFront = decay_t<front_t<Args...>>,
		typename = enable_if_t<
			(sizeof...(Args) > 0) &&
			std::is_constructible<T, Args &&...>::value &&
			!std::is_base_of<skater, ArgsFront>::value>>
	skater(Args &&...args) {
		new (&value()) T(std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args &&...>::
				value>>
	skater(std::initializer_list<U> il, Args &&...args) {
		new (&value()) T(il, std::forward<Args>(args)...);
	}

	~skater() {
		value().~T();
	}

	skater(skater &&src) {
		new (&value()) T(std::move(src.value()));
	}

	skater(const skater &src) : skater(std::move(const_cast<skater &>(src))) {}

	RUA_OVERLOAD_ASSIGNMENT(skater)

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

private:
	alignas(alignof(T)) uchar _sto[sizeof(T)];
};

template <typename T, typename Skater = skater<decay_t<T>>>
Skater make_skater(T &&val) {
	return Skater(std::forward<T>(val));
}

template <typename T, typename Skater = skater<decay_t<T>>>
Skater skate(T &&val) {
	return Skater(std::move(val));
}

} // namespace rua

#endif
