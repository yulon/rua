#ifndef _RUA_STRING_TO_STRING_HPP
#define _RUA_STRING_TO_STRING_HPP

#include "encoding/base.hpp"
#include "string_view.hpp"

#include "../generic_ptr.hpp"
#include "../macros.hpp"
#include "../type_traits/std_patch.hpp"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace rua {

template <typename T>
RUA_FORCE_INLINE enable_if_t<
	!std::is_same<decay_t<T>, bool>::value,
	decltype(std::to_string(std::declval<decay_t<T>>()))>
to_string(T &&val) {
	return std::to_string(std::forward<T>(val));
}

RUA_FORCE_INLINE const char *to_string(std::nullptr_t) {
	static const auto null_str = "null";
	return null_str;
}

RUA_FORCE_INLINE std::string to_string(string_view sv) {
	return std::string(sv.data(), sv.size());
}

RUA_FORCE_INLINE std::string to_string(wstring_view wsv) {
	return w_to_u8(wsv.data());
}

RUA_FORCE_INLINE const std::string &to_string(const std::string &s) {
	return s;
}

RUA_FORCE_INLINE std::string &&to_string(std::string &&s) {
	return std::move(s);
}

template <
	typename T,
	typename = enable_if_t<!std::is_same<decay_t<T>, unsigned char>::value>>
RUA_FORCE_INLINE std::string to_hex(T val, size_t width = sizeof(T) * 2) {
	std::stringstream ss;
	ss << "0x" << std::hex << std::uppercase << std::setw(width)
	   << std::setfill('0') << val;
	return ss.str();
}

RUA_FORCE_INLINE std::string
to_hex(unsigned char val, size_t width = sizeof(unsigned char) * 2) {
	return to_hex(static_cast<uintptr_t>(val), width);
}

RUA_FORCE_INLINE std::string to_string(generic_ptr ptr) {
	return ptr ? to_hex(ptr.uintptr()) : to_string(nullptr);
}

template <typename Bool>
RUA_FORCE_INLINE enable_if_t<std::is_same<Bool, bool>::value, const char *>
to_string(Bool val) {
	static const auto true_c_str = "true";
	static const auto false_c_str = "false";
	return val ? true_c_str : false_c_str;
}

////////////////////////////////////////////////////////////////////////////

RUA_FORCE_INLINE string_view to_temp_string(string_view sv) {
	return sv;
}

template <typename T>
RUA_FORCE_INLINE enable_if_t<
	!std::is_convertible<T, string_view>::value &&
		!std::is_same<decltype(to_string(std::declval<T>())), const char *>::
			value,
	std::string>
to_temp_string(T &&src) {
	return to_string(std::forward<T>(src));
}

template <typename T>
RUA_FORCE_INLINE enable_if_t<
	!std::is_convertible<T, string_view>::value &&
		std::is_same<decltype(to_string(std::declval<T>())), const char *>::
			value,
	const char *>
to_temp_string(T &&src) {
	return to_string(std::forward<T>(src));
}

} // namespace rua

#endif
