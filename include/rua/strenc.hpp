#ifndef _RUA_STRENC_HPP
#define _RUA_STRENC_HPP

#ifdef _WIN32
	#include <windows.h>
#endif

#include <string>

namespace rua {
	#ifdef _WIN32
		inline std::wstring u8_to_w(const std::string &u8_str) {
			if (u8_str.empty()) {
				return L"";
			}

			int w_str_len = MultiByteToWideChar(CP_UTF8, 0, u8_str.c_str(), -1, NULL, 0);
			if (!w_str_len) {
				return L"";
			}

			wchar_t *w_c_str = new wchar_t[w_str_len + 1];
			MultiByteToWideChar(CP_UTF8, 0, u8_str.c_str(), -1, w_c_str, w_str_len);

			std::wstring w_str(w_c_str);
			delete[] w_c_str;
			return w_str;
		}

		inline std::string w_to_u8(const std::wstring &w_str) {
			if (w_str.empty()) {
				return "";
			}

			int u8_str_len = WideCharToMultiByte(CP_UTF8, 0, w_str.c_str(), -1, NULL, 0, NULL, NULL);
			if (!u8_str_len) {
				return "";
			}

			char *u8_c_str = new char[u8_str_len + 1];
			u8_c_str[u8_str_len] = 0;
			WideCharToMultiByte(CP_UTF8, 0, w_str.c_str(), -1, u8_c_str, u8_str_len, NULL, NULL);

			std::string u8_str(u8_c_str);
			delete[] u8_c_str;
			return u8_str;
		}
	#endif
}

#endif
