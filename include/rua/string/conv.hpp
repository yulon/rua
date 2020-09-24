#ifndef _RUA_STRING_CONV_HPP
#define _RUA_STRING_CONV_HPP

#include "encoding/base.hpp"
#include "view.hpp"

#include "../generic_ptr.hpp"
#include "../macros.hpp"
#include "../types/traits.hpp"
#include "../types/util.hpp"

#include <iomanip>
#include <sstream>
#include <string>

namespace rua {

template <typename T>
inline enable_if_t<
	!std::is_same<decay_t<T>, bool>::value,
	decltype(std::to_string(std::declval<decay_t<T>>()))>
to_string(T &&val) {
	return std::to_string(std::forward<T>(val));
}

inline const char *to_string(std::nullptr_t) {
	static const auto null_str = "null";
	return null_str;
}

inline std::string to_string(const char *c_str) {
	return std::string(c_str);
}

inline std::string to_string(string_view sv) {
	return std::string(sv.data(), sv.size());
}

inline std::string to_string(wstring_view wsv) {
	return w_to_u8(wsv.data());
}

inline const std::string &to_string(const std::string &s) {
	return s;
}

inline std::string &&to_string(std::string &&s) {
	return std::move(s);
}

template <
	typename T,
	typename = enable_if_t<!std::is_same<decay_t<T>, unsigned char>::value>>
inline std::string to_hex(T val, size_t width = sizeof(T) * 2) {
	std::stringstream ss;
	ss << "0x" << std::hex << std::uppercase << std::setw(width)
	   << std::setfill('0') << val;
	return ss.str();
}

inline std::string
to_hex(unsigned char val, size_t width = sizeof(unsigned char) * 2) {
	return to_hex(static_cast<uintptr_t>(val), width);
}

inline std::string to_string(generic_ptr ptr) {
	return ptr ? to_hex(ptr.uintptr()) : to_string(nullptr);
}

template <typename Bool>
inline enable_if_t<std::is_same<Bool, bool>::value, const char *>
to_string(Bool val) {
	static const auto true_c_str = "true";
	static const auto false_c_str = "false";
	return val ? true_c_str : false_c_str;
}

template <typename... Args>
constexpr inline enable_if_t<
	std::is_constructible<string_view, Args &&...>::value,
	string_view>
as_string(Args &&... args) {
	return string_view(std::forward<Args>(args)...);
}

template <typename... Args>
constexpr inline enable_if_t<
	std::is_constructible<string_view, Args &&...>::value,
	string_view>
to_temp_string(Args &&... args) {
	return string_view(std::forward<Args>(args)...);
}

template <typename... Args>
inline enable_if_t<
	!std::is_constructible<string_view, Args &&...>::value,
	decltype(to_string(std::declval<Args &&>()...))>
to_temp_string(Args &&... args) {
	return to_string(std::forward<Args>(args)...);
}

} // namespace rua

#endif
