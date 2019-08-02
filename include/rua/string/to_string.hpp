#ifndef _RUA_STRING_TO_STRING_HPP
#define _RUA_STRING_TO_STRING_HPP

#include "encoding/base.hpp"

#include "../macros.hpp"

#include <string>

#ifdef __cpp_lib_string_view
#include <string_view>
#endif

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <type_traits>

namespace rua {

template <typename T>
inline std::string to_string(
	T &&val,
	decltype(std::to_string(std::declval<typename std::decay<T>::type>())) * =
		nullptr,
	typename std::enable_if<
		!std::is_unsigned<typename std::decay<T>::type>::value>::type * =
		nullptr) {
	return std::to_string(std::forward<T>(val));
}

inline std::string to_string(std::nullptr_t) {
	return "null";
}

inline std::string to_string(const char *val) {
	return val ? val : to_string(nullptr);
}

inline std::string to_string(std::string val) {
	return val;
}

inline std::string to_string(const wchar_t *val) {
	return val ? w_to_u8(val) : to_string(nullptr);
}

inline std::string to_string(std::wstring val) {
	return w_to_u8(val);
}

#ifdef __cpp_lib_string_view

inline std::string to_string(std::string_view val) {
	return val.data();
}

inline std::string to_string(std::wstring_view val) {
	return w_to_u8(val.data());
}

#endif

template <
	typename T,
	typename = typename std::enable_if<
		!std::is_same<typename std::decay<T>::type, unsigned char>::value>::
		type>
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

template <typename T>
inline std::string to_string(
	T &&val,
	typename std::enable_if<
		std::is_unsigned<typename std::decay<T>::type>::value &&
		!std::is_same<typename std::decay<T>::type, bool>::value>::type * =
		nullptr) {
	return to_hex(std::forward<T>(val));
}

inline std::string to_string(const void *val) {
	return val ? to_string(reinterpret_cast<uintptr_t>(val))
			   : to_string(nullptr);
}

inline std::string to_string(bool val) {
	return val ? "true" : "false";
}

} // namespace rua

#endif
