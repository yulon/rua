#ifndef _RUA_GENERIC_PTR_HPP
#define _RUA_GENERIC_PTR_HPP

#include "macros.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace rua {

class generic_ptr {
public:
	generic_ptr() = default;

	RUA_FORCE_INLINE constexpr generic_ptr(std::nullptr_t) : _val(0) {}

	template <typename T>
	RUA_FORCE_INLINE generic_ptr(T *src) :
		_val(reinterpret_cast<uintptr_t>(src)) {}

	template <
		typename T,
		typename = typename std::enable_if<
			std::is_member_function_pointer<T>::value>::type>
	RUA_FORCE_INLINE generic_ptr(const T &src) :
		generic_ptr(*reinterpret_cast<const void *const *>(&src)) {}

	RUA_FORCE_INLINE explicit constexpr generic_ptr(uintptr_t val) :
		_val(val) {}

	RUA_FORCE_INLINE uintptr_t &integer() {
		return _val;
	}

	RUA_FORCE_INLINE constexpr uintptr_t integer() const {
		return _val;
	}

	template <typename T>
	RUA_FORCE_INLINE typename std::enable_if<std::is_pointer<T>::value, T>::type
	as() const {
		return reinterpret_cast<T>(_val);
	}

	template <typename T>
	RUA_FORCE_INLINE typename std::
		enable_if<std::is_member_function_pointer<T>::value, T>::type
		as() const {
		return *reinterpret_cast<T *>(&_val);
	}

	template <
		typename T,
		typename = typename std::enable_if<
			std::is_pointer<T>::value ||
			std::is_member_function_pointer<T>::value>::type>
	RUA_FORCE_INLINE operator T() const {
		return as<T>();
	}

	RUA_FORCE_INLINE constexpr explicit operator uintptr_t() const {
		return _val;
	}

	RUA_FORCE_INLINE constexpr operator bool() const {
		return _val;
	}

	RUA_FORCE_INLINE bool operator>(const generic_ptr target) const {
		return _val > target._val;
	}

	RUA_FORCE_INLINE bool operator<(const generic_ptr target) const {
		return _val < target._val;
	}

	RUA_FORCE_INLINE bool operator>=(const generic_ptr target) const {
		return _val >= target._val;
	}

	RUA_FORCE_INLINE bool operator<=(const generic_ptr target) const {
		return _val <= target._val;
	}

	RUA_FORCE_INLINE bool operator==(const generic_ptr target) const {
		return _val == target._val;
	}

	RUA_FORCE_INLINE generic_ptr &operator++() {
		++_val;
		return *this;
	}

	RUA_FORCE_INLINE generic_ptr operator++() const {
		return generic_ptr(_val + 1);
	}

	RUA_FORCE_INLINE generic_ptr operator++(int) {
		return generic_ptr(_val++);
	}

	RUA_FORCE_INLINE generic_ptr &operator--() {
		--_val;
		return *this;
	}

	RUA_FORCE_INLINE generic_ptr operator--() const {
		return generic_ptr(_val - 1);
	}

	RUA_FORCE_INLINE generic_ptr operator--(int) {
		return generic_ptr(_val--);
	}

	template <typename Int>
	RUA_FORCE_INLINE typename std::enable_if<
		std::is_integral<typename std::decay<Int>::type>::value,
		generic_ptr>::type
	operator+(Int &&target) const {
		return generic_ptr(_val + static_cast<uintptr_t>(target));
	}

	RUA_FORCE_INLINE ptrdiff_t operator-(const generic_ptr target) const {
		return static_cast<ptrdiff_t>(_val - target._val);
	}

	template <typename Int>
	RUA_FORCE_INLINE typename std::enable_if<
		std::is_integral<typename std::decay<Int>::type>::value,
		generic_ptr>::type
	operator<<(Int &&target) const {
		return generic_ptr(_val << static_cast<uintptr_t>(target));
	}

	template <typename Int>
	RUA_FORCE_INLINE typename std::enable_if<
		std::is_integral<typename std::decay<Int>::type>::value,
		generic_ptr>::type
	operator>>(Int &&target) const {
		return generic_ptr(_val >> static_cast<uintptr_t>(target));
	}

	template <typename Int>
	RUA_FORCE_INLINE typename std::enable_if<
		std::is_integral<typename std::decay<Int>::type>::value,
		generic_ptr>::type
	operator|(const generic_ptr target) const {
		return generic_ptr(_val | target._val);
	}

	template <typename Int>
	RUA_FORCE_INLINE typename std::enable_if<
		std::is_integral<typename std::decay<Int>::type>::value,
		generic_ptr>::type
	operator&(const generic_ptr target) const {
		return generic_ptr(_val & target._val);
	}

	template <typename Int>
	RUA_FORCE_INLINE typename std::enable_if<
		std::is_integral<typename std::decay<Int>::type>::value,
		generic_ptr>::type
	operator^(const generic_ptr target) const {
		return generic_ptr(_val ^ target._val);
	}

	RUA_FORCE_INLINE generic_ptr operator~() const {
		return generic_ptr(~_val);
	}

	template <typename Int>
	RUA_FORCE_INLINE typename std::enable_if<
		std::is_integral<typename std::decay<Int>::type>::value,
		generic_ptr>::type
	operator-(Int &&target) const {
		return generic_ptr(_val - static_cast<uintptr_t>(target));
	}

	template <typename Int>
	RUA_FORCE_INLINE typename std::enable_if<
		std::is_integral<typename std::decay<Int>::type>::value,
		generic_ptr &>::type
	operator+=(Int &&target) {
		_val += static_cast<uintptr_t>(target);
		return *this;
	}

	template <typename Int>
	RUA_FORCE_INLINE typename std::enable_if<
		std::is_integral<typename std::decay<Int>::type>::value,
		generic_ptr &>::type
	operator-=(Int &&target) {
		_val -= static_cast<uintptr_t>(target);
		return *this;
	}

	template <typename Int>
	RUA_FORCE_INLINE typename std::enable_if<
		std::is_integral<typename std::decay<Int>::type>::value,
		generic_ptr &>::type
	operator<<=(Int &&target) {
		_val <<= static_cast<uintptr_t>(target);
		return *this;
	}

	template <typename Int>
	RUA_FORCE_INLINE typename std::enable_if<
		std::is_integral<typename std::decay<Int>::type>::value,
		generic_ptr &>::type
	operator>>=(Int &&target) {
		_val >>= static_cast<uintptr_t>(target);
		return *this;
	}

	RUA_FORCE_INLINE generic_ptr &operator|=(const generic_ptr target) {
		_val |= target._val;
		return *this;
	}

	RUA_FORCE_INLINE generic_ptr &operator&=(const generic_ptr target) {
		_val &= target._val;
		return *this;
	}

	RUA_FORCE_INLINE generic_ptr &operator^=(const generic_ptr target) {
		_val ^= target._val;
		return *this;
	}

private:
	uintptr_t _val;
};

} // namespace rua

#endif
