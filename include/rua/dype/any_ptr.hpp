#ifndef _rua_dype_any_ptr_hpp
#define _rua_dype_any_ptr_hpp

#include "../string/conv.hpp"
#include "../util.hpp"

#include <cstring>
#include <functional>

namespace rua {

class any_ptr {
public:
	any_ptr() = default;

	constexpr any_ptr(std::nullptr_t) : $val(0) {}

	template <typename T>
	any_ptr(T *src) : $val(reinterpret_cast<uintptr_t>(src)) {}

	template <typename M, typename T>
	any_ptr(M T::*src) {
		memcpy(&$val, &src, sizeof(uintptr_t));
	}

	explicit constexpr any_ptr(uintptr_t val) : $val(val) {}

	constexpr uintptr_t uintptr() const {
		return $val;
	}

	template <typename T>
	enable_if_t<std::is_pointer<T>::value, T> as() const {
		return reinterpret_cast<T>($val);
	}

	template <typename T>
	enable_if_t<std::is_member_pointer<T>::value, T> as() const {
		T mem_ptr;
		memcpy(&mem_ptr, &$val, sizeof(uintptr_t));
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
		return $val;
	}

	constexpr operator bool() const {
		return $val;
	}

	constexpr bool operator>(const any_ptr target) const {
		return $val > target.$val;
	}

	constexpr bool operator<(const any_ptr target) const {
		return $val < target.$val;
	}

	bool operator>=(const any_ptr target) const {
		return $val >= target.$val;
	}

	bool operator<=(const any_ptr target) const {
		return $val <= target.$val;
	}

	constexpr bool operator==(const any_ptr target) const {
		return $val == target.$val;
	}

	constexpr bool operator!=(const any_ptr target) const {
		return $val != target.$val;
	}

	any_ptr &operator++() {
		++$val;
		return *this;
	}

	constexpr any_ptr operator++() const {
		return any_ptr($val + 1);
	}

	any_ptr operator++(int) {
		return any_ptr($val++);
	}

	any_ptr &operator--() {
		--$val;
		return *this;
	}

	constexpr any_ptr operator--() const {
		return any_ptr($val - 1);
	}

	any_ptr operator--(int) {
		return any_ptr($val--);
	}

	template <typename Int>
	constexpr enable_if_t<std::is_integral<decay_t<Int>>::value, any_ptr>
	operator+(Int &&target) const {
		return any_ptr($val + static_cast<uintptr_t>(target));
	}

	template <typename Int>
	enable_if_t<std::is_integral<decay_t<Int>>::value, any_ptr &>
	operator+=(Int &&target) {
		$val += static_cast<uintptr_t>(target);
		return *this;
	}

	constexpr ptrdiff_t operator-(const any_ptr target) const {
		return static_cast<ptrdiff_t>($val - target.$val);
	}

	template <typename Int>
	constexpr enable_if_t<std::is_integral<decay_t<Int>>::value, any_ptr>
	operator-(Int &&target) const {
		return any_ptr($val - static_cast<uintptr_t>(target));
	}

	template <typename Int>
	enable_if_t<std::is_integral<decay_t<Int>>::value, any_ptr &>
	operator-=(Int &&target) {
		$val -= static_cast<uintptr_t>(target);
		return *this;
	}

	template <typename Int>
	constexpr enable_if_t<std::is_integral<decay_t<Int>>::value, ptrdiff_t>
	operator/(Int &&target) const {
		return static_cast<ptrdiff_t>($val / static_cast<uintptr_t>(target));
	}

	template <typename Int>
	constexpr enable_if_t<std::is_integral<decay_t<Int>>::value, ptrdiff_t>
	operator%(Int &&target) const {
		return static_cast<ptrdiff_t>($val % static_cast<uintptr_t>(target));
	}

	template <typename Int>
	constexpr enable_if_t<std::is_integral<decay_t<Int>>::value, any_ptr>
	operator<<(Int &&target) const {
		return any_ptr($val << static_cast<uintptr_t>(target));
	}

	template <typename Int>
	enable_if_t<std::is_integral<decay_t<Int>>::value, any_ptr &>
	operator<<=(Int &&target) {
		$val <<= static_cast<uintptr_t>(target);
		return *this;
	}

	template <typename Int>
	constexpr enable_if_t<std::is_integral<decay_t<Int>>::value, any_ptr>
	operator>>(Int &&target) const {
		return any_ptr($val >> static_cast<uintptr_t>(target));
	}

	template <typename Int>

	enable_if_t<std::is_integral<decay_t<Int>>::value, any_ptr &>
	operator>>=(Int &&target) {
		$val >>= static_cast<uintptr_t>(target);
		return *this;
	}

	any_ptr operator|(const any_ptr target) const {
		return any_ptr($val | target.$val);
	}

	any_ptr &operator|=(const any_ptr target) {
		$val |= target.$val;
		return *this;
	}

	any_ptr operator&(const any_ptr target) const {
		return any_ptr($val & target.$val);
	}

	any_ptr &operator&=(const any_ptr target) {
		$val &= target.$val;
		return *this;
	}

	any_ptr operator^(const any_ptr target) const {
		return any_ptr($val ^ target.$val);
	}

	any_ptr &operator^=(const any_ptr target) {
		$val ^= target.$val;
		return *this;
	}

	constexpr any_ptr operator~() const {
		return any_ptr(~$val);
	}

private:
	uintptr_t $val;
};

inline std::string to_string(const any_ptr ptr) {
	return ptr ? to_hex(ptr.uintptr()) : to_string(nullptr);
}

} // namespace rua

namespace std {

template <>
struct hash<rua::any_ptr> {
	constexpr size_t operator()(const rua::any_ptr p) const {
		return static_cast<size_t>(p.uintptr());
	}
};

} // namespace std

#endif
