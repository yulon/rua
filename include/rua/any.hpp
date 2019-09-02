#ifndef _RUA_ANY_HPP
#define _RUA_ANY_HPP

#include "any_word.hpp"
#include "macros.hpp"
#include "type_traits.hpp"

#include <cassert>

namespace rua {

class any {
public:
	constexpr any() : _val(), _typ_inf(nullptr) {}

	template <
		typename T,
		typename DecayT = typename std::decay<T>::type,
		typename =
			typename std::enable_if<!std::is_base_of<any, DecayT>::value>::type>
	any(T &&src) : _val(std::forward<T>(src)), _typ_inf(&type_info<DecayT>()) {}

	template <
		typename T,
		typename... Args,
		typename = typename std::enable_if<
			!std::is_same<T, void>::value &&
			!std::is_base_of<any, T>::value>::type>
	any(in_place_type_t<T> iptt, Args &&... args) :
		_val(iptt, std::forward<Args>(args)...),
		_typ_inf(&type_info<T>()) {}

	template <
		typename T,
		typename U,
		typename... Args,
		typename = typename std::enable_if<
			!std::is_same<T, void>::value &&
			!std::is_base_of<any, T>::value>::type>
	any(in_place_type_t<T> iptt, std::initializer_list<U> il, Args &&... args) :
		_val(iptt, il, std::forward<Args>(args)...),
		_typ_inf(&type_info<T>()) {}

	any(in_place_type_t<void>) : any() {}

	~any() {
		reset();
	}

	any(const any &src) : _typ_inf(src._typ_inf) {
		if (!_typ_inf) {
			return;
		}
		assert(_typ_inf->copy_from_any_word);
		_val = _typ_inf->copy_from_any_word(src._val);
	}

	any(any &&src) : _val(src._val), _typ_inf(src._typ_inf) {
		if (!src._typ_inf) {
			return;
		}
		assert(_typ_inf->move_ctor);
		src._typ_inf = nullptr;
	}

	RUA_OVERLOAD_ASSIGNMENT(any)

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
	decltype(std::declval<any_word &>().as<T>()) as() & {
		return _val.as<T>();
	}

	template <typename T>
	decltype(std::declval<any_word>().as<T>()) as() && {
		return std::move(_val).as<T>();
	}

	template <typename T>
	decltype(std::declval<const any_word &>().as<T>()) as() const & {
		return _val.as<T>();
	}

	template <
		typename... Args,
		typename = typename std::enable_if<
			std::is_constructible<any, Args...>::value>::type>
	void emplace(Args &&... args) {
		reset();
		new (this) any(std::forward<Args>(args)...);
	}

	template <
		typename T,
		typename U,
		typename... Args,
		typename = typename std::enable_if<
			std::is_constructible<any, std::initializer_list<U>, Args...>::
				value>::type>
	void emplace(
		in_place_type_t<T> iptt, std::initializer_list<U> il, Args &&... args) {
		reset();
		new (this) any(iptt, il, std::forward<Args>(args)...);
	}

	void reset() {
		if (!_typ_inf) {
			return;
		}
		if (_typ_inf->dtor_from_any_word) {
			_typ_inf->dtor_from_any_word(_val);
		}
		_typ_inf = nullptr;
	}

private:
	any_word _val;
	const type_info_t *_typ_inf;
};

template <class T, class... Args>
any make_any(Args &&... args) {
	return any(in_place_type_t<T>{}, std::forward<Args>(args)...);
}

template <class T, class U, class... Args>
any make_any(std::initializer_list<U> il, Args &&... args) {
	return any(in_place_type_t<T>{}, il, std::forward<Args>(args)...);
}

} // namespace rua

#endif
