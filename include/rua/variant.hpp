#ifndef _RUA_VARIANT_HPP
#define _RUA_VARIANT_HPP

#include "types.hpp"

#include <cassert>

namespace rua {

template <typename... Types>
class variant {
public:
	constexpr variant() : _sto(), _typ_inf(nullptr) {}

	template <
		typename T,
		typename =
			enable_if_t<index_of<decay_t<T>, Types...>::value != nullindex>>
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

	~variant() {
		reset();
	}

	variant(const variant &src) : _typ_inf(src._typ_inf) {
		if (!_typ_inf) {
			return;
		}
		assert(_typ_inf->copy_ctor);
		_typ_inf->copy_ctor(&_sto[0], &src._sto);
	}

	variant(variant &&src) : _typ_inf(src._typ_inf) {
		if (!_typ_inf) {
			return;
		}
		assert(_typ_inf->move_ctor);
		_typ_inf->move_ctor(&_sto[0], &src._sto);
	}

	RUA_OVERLOAD_ASSIGNMENT(variant)

	bool has_value() const {
		return !type_is<void>();
	}

	operator bool() const {
		return has_value();
	}

	type_id_t type() const {
		return _typ_inf ? _typ_inf->id : type_id<void>();
	}

	template <typename T>
	bool type_is() const {
		return type() == type_id<T>();
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
		if (!_typ_inf) {
			return;
		}
		if (_typ_inf->dtor) {
			_typ_inf->dtor(reinterpret_cast<void *>(&_sto[0]));
		}
		_typ_inf = nullptr;
	}

private:
	alignas(
		max_align_of<Types...>::value) char _sto[max_size_of<Types...>::value];
	const type_info_t *_typ_inf;

	template <typename T, typename... Args>
	T &_emplace(Args &&... args) {
		RUA_SPASSERT((index_of<decay_t<T>, Types...>::value != nullindex));

		_typ_inf = &type_info<T>();
		return *(new (&_sto[0]) T(std::forward<Args>(args)...));
	}
};

template <typename T, typename... Types, typename... Args>
variant<Types...> make_variant(Args &&... args) {
	return variant<Types...>(in_place_type_t<T>{}, std::forward<Args>(args)...);
}

template <typename T, typename... Types, typename U, typename... Args>
variant<Types...> make_variant(std::initializer_list<U> il, Args &&... args) {
	return variant<Types...>(
		in_place_type_t<T>{}, il, std::forward<Args>(args)...);
}

} // namespace rua

#endif
