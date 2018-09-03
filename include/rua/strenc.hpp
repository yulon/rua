#ifndef _RUA_STRENC_HPP
#define _RUA_STRENC_HPP

#ifdef _WIN32
	#include <windows.h>
#endif

#include <string>

namespace rua {
	#ifdef _WIN32
		inline std::wstring _mb_to_w(UINT mb_cp, const std::string &mb_str) {
			if (mb_str.empty()) {
				return L"";
			}

			int w_str_len = MultiByteToWideChar(mb_cp, 0, mb_str.c_str(), -1, NULL, 0);
			if (!w_str_len) {
				return L"";
			}

			wchar_t *w_c_str = new wchar_t[w_str_len + 1];
			MultiByteToWideChar(mb_cp, 0, mb_str.c_str(), -1, w_c_str, w_str_len);

			std::wstring w_str(w_c_str);
			delete[] w_c_str;
			return w_str;
		}

		inline std::string _w_to_mb(const std::wstring &w_str, UINT mb_cp) {
			if (w_str.empty()) {
				return "";
			}

			int u8_str_len = WideCharToMultiByte(mb_cp, 0, w_str.c_str(), -1, NULL, 0, NULL, NULL);
			if (!u8_str_len) {
				return "";
			}

			char *u8_c_str = new char[u8_str_len + 1];
			u8_c_str[u8_str_len] = 0;
			WideCharToMultiByte(mb_cp, 0, w_str.c_str(), -1, u8_c_str, u8_str_len, NULL, NULL);

			std::string u8_str(u8_c_str);
			delete[] u8_c_str;
			return u8_str;
		}

		inline std::wstring u8_to_w(const std::string &u8_str) {
			return _mb_to_w(CP_UTF8, u8_str);
		}

		inline std::string w_to_u8(const std::wstring &w_str) {
			return _w_to_mb(w_str, CP_UTF8);;
		}

		inline std::wstring l_to_w(const std::string &str) {
			return _mb_to_w(CP_ACP, str);
		}

		inline std::string w_to_l(const std::wstring &w_str) {
			return _w_to_mb(w_str, CP_ACP);;
		}

		inline std::string l_to_u8(const std::string &str) {
			auto w_str = l_to_w(str);
			return w_to_u8(w_str);
		}

		inline std::string u8_to_l(const std::string &u8_str) {
			auto w_str = u8_to_w(u8_str);
			return w_to_l(w_str);
		}

		#define RUA_L_TO_U8(str) ::rua::l_to_u8(str)
		#define RUA_U8_TO_L(u8_str) ::rua::u8_to_l(u8_str)
	#else
		inline std::string l_to_u8(const std::string &str) {
			return str;
		}

		inline std::string u8_to_l(const std::string &u8_str) {
			return u8_str;
		}

		#define RUA_L_TO_U8(str) str
		#define RUA_U8_TO_L(u8_str) u8_str
	#endif
}

#endif
