#ifndef _RUA_STRING_ENCODING_BASE_HPP
#define _RUA_STRING_ENCODING_BASE_HPP

#ifdef _WIN32

#include "base/win32.hpp"

namespace rua {

inline std::string loc_to_u8(const std::string &str) {
	return win32::loc_to_u8(str);
}

inline std::string u8_to_loc(const std::string &u8_str) {
	return win32::u8_to_loc(u8_str);
}

inline std::wstring u8_to_w(const std::string &u8_str) {
	return win32::u8_to_w(u8_str);
}

inline std::string w_to_u8(const std::wstring &w_str) {
	return win32::w_to_u8(w_str);
}

} // namespace rua

#define RUA_LOC_TO_U8(str) (::rua::loc_to_u8(str))
#define RUA_U8_TO_LOC(u8_str) (::rua::u8_to_loc(u8_str))

#else

#include "base/uni.hpp"

namespace rua {

inline std::string loc_to_u8(const std::string &str) {
	return uni::loc_to_u8(str);
}

inline std::string u8_to_loc(const std::string &u8_str) {
	return uni::u8_to_loc(u8_str);
}

inline std::wstring u8_to_w(const std::string &u8_str) {
	return uni::u8_to_w(u8_str);
}

inline std::string w_to_u8(const std::wstring &w_str) {
	return uni::w_to_u8(w_str);
}

} // namespace rua

#define RUA_LOC_TO_U8(str) (str)
#define RUA_U8_TO_LOC(u8_str) (u8_str)

#endif

#endif
