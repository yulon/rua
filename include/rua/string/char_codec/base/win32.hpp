#ifndef _RUA_STRING_CHAR_CODEC_BASE_WIN32_HPP
#define _RUA_STRING_CHAR_CODEC_BASE_WIN32_HPP

#include "../../view.hpp"

#include "../../../macros.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <string>

namespace rua { namespace win32 {

namespace _string_char_codec_base {

inline std::wstring b2w(UINT mb_cp, string_view mb_str) {
	if (mb_str.empty()) {
		return L"";
	}

	int w_str_len = MultiByteToWideChar(
		mb_cp, 0, mb_str.data(), static_cast<int>(mb_str.size()), nullptr, 0);
	if (!w_str_len) {
		return L"";
	}

	wchar_t *w_c_str = new wchar_t[w_str_len + 1];
	MultiByteToWideChar(
		mb_cp,
		0,
		mb_str.data(),
		static_cast<int>(mb_str.size()),
		w_c_str,
		w_str_len);

	std::wstring w_str(w_c_str, w_str_len);
	delete[] w_c_str;
	return w_str;
}

inline std::string w2b(wstring_view w_str, UINT mb_cp) {
	if (w_str.empty()) {
		return "";
	}

	int u8_str_len = WideCharToMultiByte(
		mb_cp,
		0,
		w_str.data(),
		static_cast<int>(w_str.size()),
		nullptr,
		0,
		nullptr,
		nullptr);
	if (!u8_str_len) {
		return "";
	}

	char *u8_c_str = new char[u8_str_len + 1];
	u8_c_str[u8_str_len] = 0;
	WideCharToMultiByte(
		mb_cp,
		0,
		w_str.data(),
		static_cast<int>(w_str.size()),
		u8_c_str,
		u8_str_len,
		nullptr,
		nullptr);

	std::string u8_str(u8_c_str, u8_str_len);
	delete[] u8_c_str;
	return u8_str;
}

inline std::wstring u2w(string_view u8_str) {
	return b2w(CP_UTF8, u8_str);
}

inline std::string w2u(wstring_view w_str) {
	return w2b(w_str, CP_UTF8);
}

inline std::wstring l2w(string_view str) {
	return b2w(CP_ACP, str);
}

inline std::string w2l(wstring_view w_str) {
	return w2b(w_str, CP_ACP);
}

inline std::string l2u(string_view str) {
	return w2u(l2w(str));
}

inline std::string u2l(string_view u8_str) {
	return w2l(u2w(u8_str));
}

} // namespace _string_char_codec_base

using namespace _string_char_codec_base;

}} // namespace rua::win32

#endif
