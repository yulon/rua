#ifndef _RUA_STRING_CONV_HPP
#define _RUA_STRING_CONV_HPP

#include "char_enc/base.hpp"
#include "view.hpp"

#include "../macros.hpp"
#include "../types/util.hpp"

#include <iomanip>
#include <sstream>
#include <string>

namespace rua {

template <typename T>
inline enable_if_t<!std::is_same<decay_t<T>, bool>::value, std::string>
to_string(T &&val) {
	return std::to_string(std::forward<T>(val));
}

inline std::string to_string(std::nullptr_t) {
	return "null";
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

inline std::string to_string(std::string s) {
	return std::string(std::move(s));
}

template <typename T>
inline enable_if_t<!std::is_same<decay_t<T>, unsigned char>::value, std::string>
to_hex(T val, size_t width = sizeof(T) * 2) {
	std::stringstream ss;
	ss << "0x" << std::hex << std::uppercase << std::setw(width)
	   << std::setfill('0') << val;
	return ss.str();
}

inline std::string
to_hex(unsigned char val, size_t width = sizeof(unsigned char) * 2) {
	return to_hex(static_cast<uintptr_t>(val), width);
}

template <typename T>
inline enable_if_t<!std::is_same<decay_t<T>, char>::value, std::string>
to_string(T *ptr) {
	return ptr ? to_hex(reinterpret_cast<uintptr_t>(ptr)) : to_string(nullptr);
}

template <typename Bool>
inline enable_if_t<std::is_same<Bool, bool>::value, std::string>
to_string(Bool val) {
	return val ? "true" : "false";
}

template <typename T, typename = void>
struct is_constructible_to_string {
	static constexpr auto value = false;
};

template <typename T>
struct is_constructible_to_string<
	T,
	void_t<decltype(to_string(std::declval<T>()))>> {
	static constexpr auto value = true;
};

template <typename Char>
constexpr inline enable_if_t<std::is_same<Char, char>::value, string_view>
as_string(const Char *c_str) {
	return c_str;
}

template <typename StringView>
inline constexpr enable_if_t<
	std::is_base_of<string_view, StringView>::value,
	string_view>
as_string(StringView sv) {
	return sv;
}

template <typename NullPtr>
inline constexpr enable_if_t<is_null_pointer<NullPtr>::value, string_view>
as_string(NullPtr) {
	return "null";
}

template <typename Bool>
inline constexpr enable_if_t<std::is_same<Bool, bool>::value, string_view>
as_string(Bool val) {
	return val ? "true" : "false";
}

template <typename T, typename = void>
struct is_constructible_as_string {
	static constexpr auto value = false;
};

template <typename T>
struct is_constructible_as_string<
	T,
	void_t<decltype(as_string(std::declval<T>()))>> {
	static constexpr auto value = true;
};

template <typename T>
constexpr inline enable_if_t<
	is_constructible_as_string<T &&>::value,
	string_view>
to_temp_string(T &&val) {
	return as_string(std::forward<T>(val));
}

template <typename T>
inline enable_if_t<
	is_constructible_to_string<T &&>::value &&
		!is_constructible_as_string<T &&>::value,
	std::string>
to_temp_string(T &&val) {
	return to_string(std::forward<T>(val));
}

} // namespace rua

#endif
