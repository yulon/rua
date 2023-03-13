#ifndef _rua_string_codec_base_uni_hpp
#define _rua_string_codec_base_uni_hpp

#include "../../view.hpp"

#include <string>

namespace rua { namespace uni {

namespace _string_codec_base {

inline std::string l2u(string_view str) {
	return std::string(str.data(), str.size());
}

inline std::string u2l(string_view u8_str) {
	return std::string(u8_str.data(), u8_str.size());
}

inline std::wstring u2w(string_view) {
	// TODO
	return L"";
}

inline std::string w2u(wstring_view) {
	// TODO
	return "";
}

} // namespace _string_codec_base

using namespace _string_codec_base;

}} // namespace rua::uni

#endif
