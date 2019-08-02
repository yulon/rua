#ifndef _RUA_STRING_ENCODING_BASE_UNI_HPP
#define _RUA_STRING_ENCODING_BASE_UNI_HPP

#include <string>

namespace rua { namespace uni {

inline std::string loc_to_u8(const std::string &str) {
	return str;
}

inline std::string u8_to_loc(const std::string &u8_str) {
	return u8_str;
}

inline std::wstring u8_to_w(const std::string &u8_str) {
	// TODO
	return L"";
}

inline std::string w_to_u8(const std::wstring &w_str) {
	// TODO
	return "";
}

}} // namespace rua::uni

#endif
