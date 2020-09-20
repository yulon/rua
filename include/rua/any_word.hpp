#ifndef _RUA_ANY_WORD_HPP
#define _RUA_ANY_WORD_HPP

#include "bit.hpp"
#include "types/traits.hpp"
#include "types/util.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <utility>

namespace rua {

class any_word {
public:
	template <typename T>
	struct is_dynamic_allocation {
		static constexpr auto value =
			!std::is_void<T>::value &&
			!(size_of<T>::value <= sizeof(uintptr_t) &&
			  std::is_trivial<T>::value) &&
			!std::is_base_of<any_word, T>::value;
	};

	////////////////////////////////////////////////////////////////////////

	any_word() = default;

	constexpr any_word(std::nullptr_t) : _val(0) {}

	template <typename T, typename = enable_if_t<std::is_integral<T>::value>>
	constexpr any_word(T val) : _val(static_cast<uintptr_t>(val)) {}

	template <typename T>
	any_word(T *ptr) : _val(reinterpret_cast<uintptr_t>(ptr)) {}

	template <
		typename T,
		typename = enable_if_t<std::is_member_function_pointer<T>::value>>
	any_word(const T &src) : any_word(reinterpret_cast<const void *>(src)) {}

	template <
		typename T,
		typename DecayT = decay_t<T>,
		typename = enable_if_t<
			!std::is_integral<DecayT>::value &&
			!std::is_pointer<DecayT>::value &&
			sizeof(DecayT) <= sizeof(uintptr_t) &&
			std::is_trivial<DecayT>::value &&
			!std::is_base_of<any_word, DecayT>::value>>
	any_word(const T &src) {
		bit_set<DecayT>(&_val, src);
	}

	template <
		typename T,
		typename DecayT = decay_t<T>,
		typename = enable_if_t<is_dynamic_allocation<DecayT>::value>>
	any_word(T &&src) :
		_val(reinterpret_cast<uintptr_t>(new DecayT(std::forward<T>(src)))) {}

	template <
		typename T,
		typename... Args,
		typename = enable_if_t<is_dynamic_allocation<T>::value>>
	any_word(in_place_type_t<T>, Args &&... args) :
		_val(reinterpret_cast<uintptr_t>(new T(std::forward<Args>(args)...))) {}

	template <
		typename T,
		typename U,
		typename... Args,
		typename = enable_if_t<is_dynamic_allocation<T>::value>>
	any_word(in_place_type_t<T>, std::initializer_list<U> il, Args &&... args) :
		_val(reinterpret_cast<uintptr_t>(
			new T(il, std::forward<Args>(args)...))) {}

	template <
		typename T,
		typename... Args,
		typename = enable_if_t<!std::is_same<T, void>::value>,
		typename = enable_if_t<!is_dynamic_allocation<T>::value>>
	any_word(in_place_type_t<T>, Args &&... args) :
		_val(T(std::forward<Args>(args)...)) {}

	any_word(in_place_type_t<void>) : _val(0) {}

	constexpr operator bool() const {
		return _val;
	}

	uintptr_t &value() {
		return _val;
	}

	constexpr uintptr_t value() const {
		return _val;
	}

	template <typename T>
	enable_if_t<std::is_integral<T>::value, T> as() const & {
		return static_cast<T>(_val);
	}

	template <typename T>
	enable_if_t<std::is_pointer<T>::value, T> as() const & {
		return reinterpret_cast<T>(_val);
	}

	template <typename T>
	enable_if_t<
		!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			(sizeof(T) <= sizeof(uintptr_t) && std::is_trivial<T>::value),
		T>
	as() const & {
		return bit_get<T>(&_val);
	}

	template <typename T>
	enable_if_t<
		!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) && std::is_trivial<T>::value),
		const T &>
	as() const & {
		return *reinterpret_cast<const T *>(_val);
	}

	template <typename T>
	enable_if_t<
		!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) && std::is_trivial<T>::value),
		T &>
	as() & {
		return *reinterpret_cast<T *>(_val);
	}

	template <typename T>
	enable_if_t<
		!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) && std::is_trivial<T>::value),
		T>
	as() && {
		auto ptr = reinterpret_cast<T *>(_val);
		T r(std::move(*ptr));
		delete ptr;
		return r;
	}

	template <
		typename T,
		typename = enable_if_t<
			std::is_integral<T>::value || std::is_pointer<T>::value ||
			(sizeof(T) <= sizeof(uintptr_t) && std::is_trivial<T>::value)>>
	operator T() const & {
		return as<T>();
	}

	template <
		typename T,
		typename = enable_if_t<
			!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) && std::is_trivial<T>::value)>>
	operator const T &() const & {
		return as<T>();
	}

	template <
		typename T,
		typename = enable_if_t<
			!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) && std::is_trivial<T>::value)>>
	operator T &() & {
		return as<T>();
	}

	template <
		typename T,
		typename = enable_if_t<
			!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) && std::is_trivial<T>::value)>>
	operator T() && {
		return std::move(*this).as<T>();
	}

	template <
		typename T,
		typename = enable_if_t<
			!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) && std::is_trivial<T>::value)>>
	void destruct() {
		delete reinterpret_cast<T *>(_val);
	}

private:
	uintptr_t _val;
};

} // namespace rua

#endif
