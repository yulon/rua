#ifndef _RUA_GENERIC_PTR_HPP
#define _RUA_GENERIC_PTR_HPP

#include "macros.hpp"
#include "string/conv.hpp"
#include "types/traits.hpp"
#include "types/util.hpp"

#include <cstring>
#include <functional>

namespace rua {

class generic_ptr {
public:
	generic_ptr() = default;

	constexpr generic_ptr(std::nullptr_t) : _val(0) {}

	template <typename T>
	generic_ptr(T *src) : _val(reinterpret_cast<uintptr_t>(src)) {}

	template <typename M, typename T>
	generic_ptr(M T::*src) {
		memcpy(&_val, &src, sizeof(uintptr_t));
	}

	explicit constexpr generic_ptr(uintptr_t val) : _val(val) {}

	constexpr uintptr_t uintptr() const {
		return _val;
	}

	template <typename T>
	enable_if_t<std::is_pointer<T>::value, T> as() const {
		return reinterpret_cast<T>(_val);
	}

	template <typename T>
	enable_if_t<std::is_member_pointer<T>::value, T> as() const {
		T mem_ptr;
		memcpy(&mem_ptr, &_val, sizeof(uintptr_t));
		return mem_ptr;
	}

	template <
		typename T,
		typename = enable_if_t<
			std::is_pointer<T>::value || std::is_member_pointer<T>::value>>
	operator T() const {
		return as<T>();
	}

	constexpr explicit operator uintptr_t() const {
		return _val;
	}

	constexpr operator bool() const {
		return _val;
	}

	constexpr bool operator>(const generic_ptr target) const {
		return _val > target._val;
	}

	constexpr bool operator<(const generic_ptr target) const {
		return _val < target._val;
	}

	bool operator>=(const generic_ptr target) const {
		return _val >= target._val;
	}

	bool operator<=(const generic_ptr target) const {
		return _val <= target._val;
	}

	constexpr bool operator==(const generic_ptr target) const {
		return _val == target._val;
	}

	constexpr bool operator!=(const generic_ptr target) const {
		return _val != target._val;
	}

	generic_ptr &operator++() {
		++_val;
		return *this;
	}

	constexpr generic_ptr operator++() const {
		return generic_ptr(_val + 1);
	}

	generic_ptr operator++(int) {
		return generic_ptr(_val++);
	}

	generic_ptr &operator--() {
		--_val;
		return *this;
	}

	constexpr generic_ptr operator--() const {
		return generic_ptr(_val - 1);
	}

	generic_ptr operator--(int) {
		return generic_ptr(_val--);
	}

	template <typename Int>
	constexpr enable_if_t<std::is_integral<decay_t<Int>>::value, generic_ptr>
	operator+(Int &&target) const {
		return generic_ptr(_val + static_cast<uintptr_t>(target));
	}

	template <typename Int>
	enable_if_t<std::is_integral<decay_t<Int>>::value, generic_ptr &>
	operator+=(Int &&target) {
		_val += static_cast<uintptr_t>(target);
		return *this;
	}

	constexpr ptrdiff_t operator-(const generic_ptr target) const {
		return static_cast<ptrdiff_t>(_val - target._val);
	}

	template <typename Int>
	constexpr enable_if_t<std::is_integral<decay_t<Int>>::value, generic_ptr>
	operator-(Int &&target) const {
		return generic_ptr(_val - static_cast<uintptr_t>(target));
	}

	template <typename Int>
	enable_if_t<std::is_integral<decay_t<Int>>::value, generic_ptr &>
	operator-=(Int &&target) {
		_val -= static_cast<uintptr_t>(target);
		return *this;
	}

	template <typename Int>
	constexpr enable_if_t<std::is_integral<decay_t<Int>>::value, ptrdiff_t>
	operator/(Int &&target) const {
		return static_cast<ptrdiff_t>(_val / static_cast<uintptr_t>(target));
	}

	template <typename Int>
	constexpr enable_if_t<std::is_integral<decay_t<Int>>::value, ptrdiff_t>
	operator%(Int &&target) const {
		return static_cast<ptrdiff_t>(_val % static_cast<uintptr_t>(target));
	}

	template <typename Int>
	constexpr enable_if_t<std::is_integral<decay_t<Int>>::value, generic_ptr>
	operator<<(Int &&target) const {
		return generic_ptr(_val << static_cast<uintptr_t>(target));
	}

	template <typename Int>
	enable_if_t<std::is_integral<decay_t<Int>>::value, generic_ptr &>
	operator<<=(Int &&target) {
		_val <<= static_cast<uintptr_t>(target);
		return *this;
	}

	template <typename Int>
	constexpr enable_if_t<std::is_integral<decay_t<Int>>::value, generic_ptr>
	operator>>(Int &&target) const {
		return generic_ptr(_val >> static_cast<uintptr_t>(target));
	}

	template <typename Int>

	enable_if_t<std::is_integral<decay_t<Int>>::value, generic_ptr &>
	operator>>=(Int &&target) {
		_val >>= static_cast<uintptr_t>(target);
		return *this;
	}

	generic_ptr operator|(const generic_ptr target) const {
		return generic_ptr(_val | target._val);
	}

	generic_ptr &operator|=(const generic_ptr target) {
		_val |= target._val;
		return *this;
	}

	generic_ptr operator&(const generic_ptr target) const {
		return generic_ptr(_val & target._val);
	}

	generic_ptr &operator&=(const generic_ptr target) {
		_val &= target._val;
		return *this;
	}

	generic_ptr operator^(const generic_ptr target) const {
		return generic_ptr(_val ^ target._val);
	}

	generic_ptr &operator^=(const generic_ptr target) {
		_val ^= target._val;
		return *this;
	}

	constexpr generic_ptr operator~() const {
		return generic_ptr(~_val);
	}

private:
	uintptr_t _val;
};

inline std::string to_string(const generic_ptr ptr) {
	return ptr ? to_hex(ptr.uintptr()) : to_string(nullptr);
}

} // namespace rua

namespace std {

template <>
struct hash<rua::generic_ptr> {
	constexpr size_t operator()(const rua::generic_ptr p) const {
		return static_cast<size_t>(p.uintptr());
	}
};

} // namespace std

#endif
