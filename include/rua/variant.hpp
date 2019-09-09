#ifndef _RUA_VARIANT_HPP
#define _RUA_VARIANT_HPP

#include "type_traits/measures.hpp"
#include "type_traits/std_patch.hpp"
#include "type_traits/type_info.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace rua {

template <typename... Types>
class variant {
public:
	constexpr variant() : _sto(), _typ_inf(nullptr) {}

	template <
		typename T,
		typename = typename std::enable_if<
			index_of<typename std::decay<T>::type, Types...>::value !=
			nullindex>::type>
	variant(T &&val) {
		_emplace<typename std::decay<T>::type>(std::forward<T>(val));
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
		_typ_inf->copy_ctor(&_sto, &src._sto);
	}

	variant(variant &&src) : _typ_inf(src._typ_inf) {
		if (!_typ_inf) {
			return;
		}
		assert(_typ_inf->move_ctor);
		_typ_inf->move_ctor(&_sto, &src._sto);
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
		return *reinterpret_cast<T *>(&_sto);
	}

	template <typename T>
	T &&as() && {
		return std::move(as<T>());
	}

	template <typename T>
	const T &as() const & {
		assert(type_is<T>());
		return *reinterpret_cast<const T *>(&_sto);
	}

	template <typename T, typename... Args>
	void emplace(Args &&... args) {
		reset();
		_emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	void emplace(std::initializer_list<U> il, Args &&... args) {
		reset();
		_emplace<T>(il, std::forward<Args>(args)...);
	}

	void reset() {
		if (!_typ_inf) {
			return;
		}
		if (_typ_inf->dtor) {
			_typ_inf->dtor(reinterpret_cast<void *>(&_sto));
		}
		_typ_inf = nullptr;
	}

private:
	alignas(
		max_align_of<Types...>::value) char _sto[max_size_of<Types...>::value];
	const type_info_t *_typ_inf;

	template <typename T, typename... Args>
	void _emplace(Args &&... args) {
		RUA_SPASSERT(
			(index_of<typename std::decay<T>::type, Types...>::value !=
			 nullindex));

		new (&_sto) T(std::forward<Args>(args)...);
		_typ_inf = &type_info<T>();
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
