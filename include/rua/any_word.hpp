#ifndef _RUA_ANY_WORD_HPP
#define _RUA_ANY_WORD_HPP

#include "bit.hpp"
#include "type_traits.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace rua {

class any_word {
public:
	any_word() = default;

	constexpr any_word(std::nullptr_t) : _val(0) {}

	template <
		typename T,
		typename = typename std::enable_if<std::is_integral<T>::value>::type>
	constexpr any_word(T val) : _val(static_cast<uintptr_t>(val)) {}

	template <typename T>
	any_word(T *ptr) : _val(reinterpret_cast<uintptr_t>(ptr)) {}

	template <
		typename T,
		typename = typename std::enable_if<
			std::is_member_function_pointer<T>::value>::type>
	any_word(const T &src) : any_word(reinterpret_cast<const void *>(src)) {}

	template <
		typename T,
		typename DecayT = typename std::decay<T>::type,
		typename = typename std::enable_if<
			!std::is_integral<DecayT>::value &&
			!std::is_pointer<DecayT>::value &&
			sizeof(DecayT) <= sizeof(uintptr_t) &&
			std::is_trivial<DecayT>::value &&
			!std::is_base_of<any_word, DecayT>::value>::type>
	any_word(const T &src) {
		bit_set<DecayT>(&_val, src);
	}

	template <
		typename T,
		typename DecayT = typename std::decay<T>::type,
		typename = typename std::enable_if<
			!std::is_integral<DecayT>::value &&
			!std::is_pointer<DecayT>::value &&
			!(sizeof(DecayT) <= sizeof(uintptr_t) &&
			  std::is_trivial<DecayT>::value) &&
			!std::is_base_of<any_word, DecayT>::value>::type>
	any_word(T &&src) :
		_val(reinterpret_cast<uintptr_t>(new DecayT(std::forward<T>(src)))) {}

	uintptr_t &value() {
		return _val;
	}

	constexpr uintptr_t value() const {
		return _val;
	}

	template <typename T>
	typename std::enable_if<std::is_integral<T>::value, T>::type as() const & {
		return static_cast<T>(_val);
	}

	template <typename T>
	typename std::enable_if<std::is_pointer<T>::value, T>::type as() const & {
		return reinterpret_cast<T>(_val);
	}

	template <typename T>
	typename std::enable_if<
		!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			(sizeof(T) <= sizeof(uintptr_t) && std::is_trivial<T>::value),
		T>::type
	as() const & {
		return bit_get<T>(&_val);
	}

	template <typename T>
	typename std::enable_if<
		!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) && std::is_trivial<T>::value),
		const T &>::type
	as() const & {
		return *reinterpret_cast<const T *>(_val);
	}

	template <typename T>
	typename std::enable_if<
		!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) && std::is_trivial<T>::value),
		T &>::type
	as() & {
		return *reinterpret_cast<T *>(_val);
	}

	template <typename T>
	typename std::enable_if<
		!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) && std::is_trivial<T>::value),
		T>::type
	as() && {
		auto ptr = reinterpret_cast<T *>(_val);
		T r(std::move(*ptr));
		delete ptr;
		return r;
	}

	template <
		typename T,
		typename = typename std::enable_if<
			std::is_integral<T>::value || std::is_pointer<T>::value ||
			(sizeof(T) <= sizeof(uintptr_t) &&
			 std::is_trivial<T>::value)>::type>
	operator T() const & {
		return as<T>();
	}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) &&
			  std::is_trivial<T>::value)>::type>
	operator const T &() const & {
		return as<T>();
	}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) &&
			  std::is_trivial<T>::value)>::type>
	operator T &() & {
		return as<T>();
	}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) &&
			  std::is_trivial<T>::value)>::type>
	operator T() && {
		return std::move(*this).as<T>();
	}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!(sizeof(T) <= sizeof(uintptr_t) &&
			  std::is_trivial<T>::value)>::type>
	void destruct() {
		delete reinterpret_cast<T *>(_val);
	}

private:
	uintptr_t _val;
};

} // namespace rua

#endif
