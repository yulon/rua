#ifndef _RUA_ANY_WORD_HPP
#define _RUA_ANY_WORD_HPP

#include "macros.hpp"
#include "type_traits.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace rua {

class any_word {
public:
	constexpr any_word() : _val(0) {}

	constexpr any_word(std::nullptr_t) : _val(0) {}

	template <
		typename T,
		typename = typename std::enable_if<std::is_integral<T>::value>::type>
	constexpr any_word(T val) : _val(static_cast<uintptr_t>(val)) {}

	template <typename T>
	constexpr any_word(T *ptr) : _val(reinterpret_cast<uintptr_t>(ptr)) {}

	template <
		typename T,
		typename DecayT = typename std::decay<T>::type,
		typename = typename std::enable_if<
			!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!std::is_base_of<any_word, DecayT>::value>::type>
	any_word(T &&src) :
		_val(reinterpret_cast<uintptr_t>(new DecayT(std::forward<T>(src)))) {}

	any_word(const any_word &src) : _val(src._val) {}

	any_word(any_word &&src) : _val(src._val) {
		if (src._val) {
			src._val = 0;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(any_word)

	uintptr_t &value() {
		return _val;
	}

	uintptr_t value() const {
		return _val;
	}

	template <typename T>
	typename std::conditional<
		std::is_integral<T>::value || std::is_pointer<T>::value,
		T,
		T &>::type
	to() & {
		return _out<T>(_out_pattern_t<T>{});
	}

	template <typename T>
	T to() && {
		return std::move(*this)._out<T>(_out_pattern_t<T>{});
	}

	template <typename T>
	typename std::conditional<
		std::is_integral<T>::value || std::is_pointer<T>::value,
		T,
		const T &>::type
	to() const & {
		return _out<T>(_out_pattern_t<T>{});
	}

	template <
		typename T,
		typename = typename std::enable_if<
			std::is_integral<T>::value || std::is_pointer<T>::value>::type>
	operator T() const & {
		return to<T>();
	}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_integral<T>::value && !std::is_pointer<T>::value>::type>
	operator T &() & {
		return to<T>();
	}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_integral<T>::value && !std::is_pointer<T>::value>::type>
	operator T() && {
		return std::move(*this).to<T>();
	}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_integral<T>::value && !std::is_pointer<T>::value>::type>
	operator const T &() const & {
		return to<T>();
	}

	template <typename T>
	void destroy() {
		if (!_val) {
			return;
		}
		_dtor<T>(_out_pattern_t<T>{});
	}

private:
	uintptr_t _val;

	struct _is_int {};
	struct _is_ptr {};

	template <typename T>
	using _out_pattern_t = switch_true_t<
		std::is_integral<T>,
		_is_int,
		std::is_pointer<T>,
		_is_ptr>;

	template <typename T>
	T _out(_is_int &&) const {
		return static_cast<T>(_val);
	}

	template <typename T>
	T _out(_is_ptr &&) const {
		return reinterpret_cast<T>(_val);
	}

	template <typename T>
	T &_out(default_t &&) & {
		return *reinterpret_cast<T *>(_val);
	}

	template <typename T>
	T _out(default_t &&) && {
		auto ptr = reinterpret_cast<T *>(_val);
		T r(std::move(*ptr));
		delete ptr;
		_val = 0;
		return r;
	}

	template <typename T>
	const T &_out(default_t &&) const & {
		return *reinterpret_cast<const T *>(_val);
	}

	template <typename T>
	void _dtor(_is_int &&) {
		_val = 0;
	}

	template <typename T>
	void _dtor(_is_ptr &&) {
		_val = 0;
	}

	template <typename T>
	void _dtor(default_t &&) {
		delete reinterpret_cast<T *>(_val);
		_val = 0;
	}
};

} // namespace rua

#endif
