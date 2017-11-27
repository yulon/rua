#ifndef _TMD_STRENC_HPP
#define _TMD_STRENC_HPP

#ifdef _WIN32
	#include <windows.h>
#endif

#include <string>

namespace tmd {
	#ifdef _WIN32
		inline std::wstring u8_to_u16(const std::string &u8_str) {
			if (u8_str.empty()) {
				return L"";
			}

			int u16_str_len = MultiByteToWideChar(CP_UTF8, 0, u8_str.c_str(), -1, NULL, 0);
			if (!u16_str_len) {
				return L"";
			}

			wchar_t *u16_c_str = new wchar_t[u16_str_len + 1];
			MultiByteToWideChar(CP_UTF8, 0, u8_str.c_str(), -1, u16_c_str, u16_str_len);

			std::wstring u16_str(u16_c_str);
			delete[] u16_c_str;
			return u16_str;
		}

		inline std::string u16_to_u8(const std::wstring &u16_str) {
			if (u16_str.empty()) {
				return "";
			}

			int u8_str_len = WideCharToMultiByte(CP_UTF8, 0, u16_str.c_str(), -1, NULL, 0, NULL, NULL);
			if (!u8_str_len) {
				return "";
			}

			char *u8_c_str = new char[u8_str_len + 1];
			u8_c_str[u8_str_len] = 0;
			WideCharToMultiByte(CP_UTF8, 0, u16_str.c_str(), -1, u8_c_str, u8_str_len, NULL, NULL);

			std::string u8_str(u8_c_str);
			delete[] u8_c_str;
			return u8_str;
		}
	#endif
}

#endif
