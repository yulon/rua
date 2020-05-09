#ifndef _RUA_VARIANT_HPP
#define _RUA_VARIANT_HPP

#include "types.hpp"

#include <cassert>

namespace rua {

template <typename... Types>
class variant : public enable_type_info {
public:
	constexpr variant() : enable_type_info(), _sto() {}

	template <
		typename T,
		typename =
			enable_if_t<index_of<decay_t<T>, Types...>::value != nullpos>>
	variant(T &&val) {
		_emplace<decay_t<T>>(std::forward<T>(val));
	}

	template <typename T, typename... Args>
	explicit variant(in_place_type_t<T>, Args &&... args) {
		_emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	explicit variant(
		in_place_type_t<T>, std::initializer_list<U> il, Args &&... args) {
		_emplace<T>(il, std::forward<Args>(args)...);
	}

	template <
		typename... Subset,
		typename = enable_if_t<
			!std::is_same<variant<Subset...>, variant>::value &&
			conjunction<bool_constant<
				index_of<Subset, Types...>::value != nullpos>...>::value>>
	variant(const variant<Subset...> &src) : enable_type_info(src.type()) {
		if (!_type) {
			return;
		}
		assert(_type.is_copyable());
		_type.copy_ctor(&_sto[0], &src.data());
	}

	template <
		typename... Subset,
		typename = enable_if_t<
			!std::is_same<variant<Subset...>, variant>::value &&
			conjunction<bool_constant<
				index_of<Subset, Types...>::value != nullpos>...>::value>>
	variant(variant<Subset...> &&src) : enable_type_info(src.type()) {
		if (!_type) {
			return;
		}
		assert(_type.is_moveable());
		_type.move_ctor(&_sto[0], &src.data());
	}

	~variant() {
		reset();
	}

	variant(const variant &src) : enable_type_info(src._type) {
		if (!_type) {
			return;
		}
		assert(_type.is_copyable());
		_type.copy_ctor(&_sto[0], &src._sto);
	}

	variant(variant &&src) : enable_type_info(src._type) {
		if (!_type) {
			return;
		}
		assert(_type.is_moveable());
		_type.move_ctor(&_sto[0], &src._sto);
	}

	RUA_OVERLOAD_ASSIGNMENT(variant)

	bool has_value() const {
		return _type;
	}

	operator bool() const {
		return _type;
	}

	template <typename T>
	T &as() & {
		assert(type_is<T>());
		return *reinterpret_cast<T *>(&_sto[0]);
	}

	template <typename T>
	T &&as() && {
		return std::move(as<T>());
	}

	template <typename T>
	const T &as() const & {
		assert(type_is<T>());
		return *reinterpret_cast<const T *>(&_sto[0]);
	}

	template <typename T, typename... Args>
	T &emplace(Args &&... args) & {
		reset();
		return _emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	T &&emplace(Args &&... args) && {
		return std::move(emplace<T>(std::forward<Args>(args)...));
	}

	template <typename T, typename U, typename... Args>
	T &emplace(std::initializer_list<U> il, Args &&... args) & {
		reset();
		return _emplace<T>(il, std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	T &&emplace(std::initializer_list<U> il, Args &&... args) && {
		return std::move(emplace<T>(il, std::forward<Args>(args)...));
	}

	void reset() {
		if (!_type) {
			return;
		}
		_type.dtor(reinterpret_cast<void *>(&_sto[0]));
		_type = type_id<void>();
	}

	byte *data() {
		return &_sto[0];
	}

	const byte *data() const {
		return &_sto[0];
	}

private:
	alignas(
		max_align_of<Types...>::value) byte _sto[max_size_of<Types...>::value];

	template <typename T, typename... Args>
	T &_emplace(Args &&... args) {
		RUA_SPASSERT((index_of<decay_t<T>, Types...>::value != nullpos));

		_type = type_id<T>();
		return *(new (&_sto[0]) T(std::forward<Args>(args)...));
	}
};

template <typename T, typename... Args>
variant<T> make_variant(Args &&... args) {
	return variant<T>(in_place_type_t<T>{}, std::forward<Args>(args)...);
}

template <typename T, typename U, typename... Args>
variant<T> make_variant(std::initializer_list<U> il, Args &&... args) {
	return variant<T>(in_place_type_t<T>{}, il, std::forward<Args>(args)...);
}

} // namespace rua

#endif
