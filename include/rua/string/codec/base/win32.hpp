#ifndef _rua_string_codec_base_win32_hpp
#define _rua_string_codec_base_win32_hpp

#include "../../view.hpp"

#include "../../../util.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <string>

namespace rua { namespace win32 {

namespace _string_codec_base {

inline std::wstring b2w(UINT mb_cp, string_view mb_str) {
	if (mb_str.empty()) {
		return L"";
	}

	int w_str_len = MultiByteToWideChar(
		mb_cp, 0, mb_str.data(), static_cast<int>(mb_str.size()), nullptr, 0);
	if (!w_str_len) {
		return L"";
	}

	std::wstring w_str(w_str_len + 1, 0);
	w_str_len = MultiByteToWideChar(
		mb_cp,
		0,
		mb_str.data(),
		static_cast<int>(mb_str.size()),
		&w_str[0],
		w_str_len);
	w_str.resize(w_str_len);
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

	std::string u8_str(u8_str_len + 1, 0);
	u8_str_len = WideCharToMultiByte(
		mb_cp,
		0,
		w_str.data(),
		static_cast<int>(w_str.size()),
		&u8_str[0],
		u8_str_len,
		nullptr,
		nullptr);
	u8_str.resize(u8_str_len);
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

} // namespace _string_codec_base

using namespace _string_codec_base;

}} // namespace rua::win32

#endif
