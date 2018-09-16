#ifndef _RUA_STRENC_HPP
#define _RUA_STRENC_HPP

#include "bin.hpp"
#include "io/util.hpp"

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

		class l_to_u8_reader : public virtual io::reader {
			public:
				l_to_u8_reader(io::reader &l_reader) : _lr(&l_reader) {}

				virtual ~l_to_u8_reader() = default;

				virtual size_t read(bin_ref p) {
					while (_cache.empty()) {
						_buf.resize(_data_sz + p.size());

						auto rsz = _lr->read(_buf);
						if (!rsz) {
							if (_data_sz) {
								_cache = l_to_u8(std::string(_buf.base().to<const char *>(), _data_sz));
							}
							break;
						}
						_data_sz += rsz;

						for (int i = _data_sz - 1; i >= 0; ++i) {
							if (static_cast<char>(_buf[static_cast<size_t>(i)]) >= 0) {
								auto valid_sz = static_cast<size_t>(i + 1);

								_cache = l_to_u8(std::string(_buf.base().to<const char *>(), valid_sz));

								_data_sz -= valid_sz;
								_buf.slice_self(valid_sz);
								break;
							}
						}
					};
					auto sz = p.copy(_cache);
					_cache = _cache.substr(sz, _cache.size() - sz);
					return sz;
				}

			private:
				io::reader *_lr;
				std::string _cache;
				bin _buf;
				size_t _data_sz;
		};

		class u8_to_l_writer : public virtual io::writer {
			public:
				u8_to_l_writer(io::writer &l_writer) : _lw(&l_writer) {}

				virtual ~u8_to_l_writer() = default;

				virtual size_t write(bin_view p) {
					_lw->write_all(u8_to_l(std::string(p.base().to<const char *>(), p.size())));
					return p.size();
				}

			private:
				io::writer *_lw;
		};

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
