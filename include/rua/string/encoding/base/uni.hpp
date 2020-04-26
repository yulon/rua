#ifndef _RUA_STRING_ENCODING_BASE_UNI_HPP
#define _RUA_STRING_ENCODING_BASE_UNI_HPP

#include "../../view.hpp"

#include <string>

namespace rua { namespace uni {

namespace _string_encoding_base {

inline std::string loc_to_u8(string_view str) {
	return std::string(str.data(), str.size());
}

inline std::string u8_to_loc(string_view u8_str) {
	return std::string(u8_str.data(), u8_str.size());
}

inline std::wstring u8_to_w(string_view) {
	// TODO
	return L"";
}

inline std::string w_to_u8(wstring_view) {
	// TODO
	return "";
}

} // namespace _string_encoding_base

using namespace _string_encoding_base;

}} // namespace rua::uni

#endif
