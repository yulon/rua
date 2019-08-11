#ifndef _RUA_STRING_ENCODING_BASE_WIN32_HPP
#define _RUA_STRING_ENCODING_BASE_WIN32_HPP

#include "../../string_view.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <string>

namespace rua { namespace win32 {

namespace _string_encoding_base {

inline std::wstring mb_to_w(UINT mb_cp, string_view mb_str) {
	if (mb_str.empty()) {
		return L"";
	}

	int w_str_len = MultiByteToWideChar(mb_cp, 0, mb_str.data(), -1, NULL, 0);
	if (!w_str_len) {
		return L"";
	}

	wchar_t *w_c_str = new wchar_t[w_str_len + 1];
	MultiByteToWideChar(mb_cp, 0, mb_str.data(), -1, w_c_str, w_str_len);

	std::wstring w_str(w_c_str);
	delete[] w_c_str;
	return w_str;
}

inline std::string w_to_mb(wstring_view w_str, UINT mb_cp) {
	if (w_str.empty()) {
		return "";
	}

	int u8_str_len =
		WideCharToMultiByte(mb_cp, 0, w_str.data(), -1, NULL, 0, NULL, NULL);
	if (!u8_str_len) {
		return "";
	}

	char *u8_c_str = new char[u8_str_len + 1];
	u8_c_str[u8_str_len] = 0;
	WideCharToMultiByte(
		mb_cp, 0, w_str.data(), -1, u8_c_str, u8_str_len, NULL, NULL);

	std::string u8_str(u8_c_str);
	delete[] u8_c_str;
	return u8_str;
}

inline std::wstring u8_to_w(string_view u8_str) {
	return mb_to_w(CP_UTF8, u8_str);
}

inline std::string w_to_u8(wstring_view w_str) {
	return w_to_mb(w_str, CP_UTF8);
}

inline std::wstring loc_to_w(string_view str) {
	return mb_to_w(CP_ACP, str);
}

inline std::string w_to_loc(wstring_view w_str) {
	return w_to_mb(w_str, CP_ACP);
}

inline std::string loc_to_u8(string_view str) {
	return w_to_u8(loc_to_w(str));
}

inline std::string u8_to_loc(string_view u8_str) {
	return w_to_loc(u8_to_w(u8_str));
}

} // namespace _string_encoding_base

using namespace _string_encoding_base;

}} // namespace rua::win32

#endif
