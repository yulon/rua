#ifndef _rua_move_only_hpp
#define _rua_move_only_hpp

#include "util.hpp"

namespace rua {

template <typename T>
class move_only : public enable_value_operators<move_only<T>, T> {
public:
	move_only() {
		construct(value());
	}

	template <
		typename... Args,
		typename ArgsFront = decay_t<front_t<Args...>>,
		typename = enable_if_t<
			(sizeof...(Args) > 0) &&
			std::is_constructible<T, Args &&...>::value &&
			!std::is_base_of<move_only, ArgsFront>::value>>
	move_only(Args &&...args) {
		construct(value(), std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args &&...>::
				value>>
	move_only(std::initializer_list<U> il, Args &&...args) {
		construct(value(), il, std::forward<Args>(args)...);
	}

	~move_only() {
		value().~T();
	}

	move_only(move_only &&src) {
		construct(value(), std::move(src.value()));
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

private:
	alignas(alignof(T)) uchar _sto[sizeof(T)];
};

template <typename T>
move_only<decay_t<T>> make_move_only(T &&val) {
	return std::move(val);
}

} // namespace rua

#endif
