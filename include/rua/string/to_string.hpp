#ifndef _RUA_STRING_TO_STRING_HPP
#define _RUA_STRING_TO_STRING_HPP

#include "encoding/base.hpp"
#include "string_view.hpp"

#include "../generic_ptr.hpp"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>

namespace rua {

template <typename T>
inline typename std::enable_if<
	!std::is_same<typename std::decay<T>::type, bool>::value,
	decltype(
		std::to_string(std::declval<typename std::decay<T>::type>()))>::type
to_string(T &&val) {
	return std::to_string(std::forward<T>(val));
}

inline const char *to_string(std::nullptr_t) {
	static const auto null_str = "null";
	return null_str;
}

inline std::string to_string(string_view sv) {
	return std::string(sv.data(), sv.size());
}

template <typename Str>
inline typename std::
	enable_if<std::is_same<Str, std::string>::value, std::string>::type
	to_string(Str &&s) {
	return std::move(s);
}

inline std::string to_string(wstring_view wsv) {
	return w_to_u8(wsv.data());
}

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

template <typename P>
inline typename std::enable_if<
	!std::is_convertible<P *, string_view>::value &&
		!std::is_convertible<P *, wstring_view>::value,
	std::string>::type
to_string(P *val) {
	return val ? to_hex(reinterpret_cast<uintptr_t>(val)) : to_string(nullptr);
}

inline std::string to_string(generic_ptr ptr) {
	return ptr ? to_hex(ptr.integer()) : to_string(nullptr);
}

inline const char *to_string(bool val) {
	static const auto true_str = "true";
	static const auto false_str = "false";
	return val ? true_str : false_str;
}

////////////////////////////////////////////////////////////////////////////

inline string_view to_temp_string(string_view sv) {
	return sv;
}

template <typename T>
inline typename std::enable_if<
	!std::is_convertible<T, string_view>::value &&
		!std::is_same<decltype(to_string(std::declval<T>())), const char *>::
			value,
	std::string>::type
to_temp_string(T &&src) {
	return to_string(std::forward<T>(src));
}

template <typename T>
inline typename std::enable_if<
	!std::is_convertible<T, string_view>::value &&
		std::is_same<decltype(to_string(std::declval<T>())), const char *>::
			value,
	const char *>::type
to_temp_string(T &&src) {
	return to_string(std::forward<T>(src));
}

} // namespace rua

#endif
