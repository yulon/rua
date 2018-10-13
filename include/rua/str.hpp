#ifndef _RUA_STR_HPP
#define _RUA_STR_HPP

#include "bin.hpp"
#include "io/util.hpp"
#include "stldata.hpp"

#ifdef _WIN32
	#include <windows.h>
#endif

#include <string>

#if RUA_CPP >= RUA_CPP_17 && RUA_HAS_INC(<string_view>)
	#include <string_view>
#endif

#include <cstring>

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
				l_to_u8_reader() : _lr(nullptr), _data_sz(0) {}

				l_to_u8_reader(io::reader &l_reader) : _lr(&l_reader), _data_sz(0) {}

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
				u8_to_l_writer() : _lw(nullptr) {}

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

	////////////////////////////////////////////////////////////////////////////

	static constexpr const char *lf = "\n";

	static constexpr const char *crlf = "\r\n";

	static constexpr const char *cr = "\r";

	#ifdef _WIN32
		static constexpr const char *eol = crlf;
	#elif RUA_DARWIN
		static constexpr const char *eol = cr;
	#else
		static constexpr const char *eol = lf;
	#endif

	inline bool is_eol(const std::string &str) {
		for (auto &chr : str) {
			if (chr != lf[0] && chr != cr[0]) {
				return false;
			}
		}
		return true;
	}

	////////////////////////////////////////////////////////////////////////////

	class line_reader {
		public:
			line_reader() : _r(nullptr), _1st_chr_is_cr(false) {}

			line_reader(io::reader &r) : _r(&r), _1st_chr_is_cr(false) {}

			std::string read_line(size_t buf_sz = 256) {
				for (;;) {
					_buf.resize(buf_sz);
					auto sz = _r->read(_buf);
					if (!sz) {
						_r = nullptr;
						_1st_chr_is_cr = false;
						return std::move(_cache);
					}
					if (_1st_chr_is_cr) {
						_1st_chr_is_cr = false;
						if (_buf[0] == static_cast<uint8_t>('\n')) {
							_buf.slice_self(1);
						}
					}
					for (size_t i = 0; i < sz; ++i) {
						if (_buf[i] == static_cast<uint8_t>('\r')) {
							_cache += std::string(_buf.base().to<const char *>(), i);
							if (i == sz - 1) {
								_1st_chr_is_cr = true;
								return std::move(_cache);
							}

							auto end = i + 1;
							if (_buf[i + 1] == static_cast<uint8_t>('\n')) {
								_1st_chr_is_cr = true;
								end = i + 1;
							} else {
								end = i;
							}

							std::string r(std::move(_cache));
							_cache = std::string((_buf.base() + end).to<const char *>(), _buf.size() - end);
							return r;

						} else if (_buf[i] == static_cast<uint8_t>('\n')) {
							_cache += std::string(_buf.base().to<const char *>(), i);
							return std::move(_cache);
						}
					}
					_cache += std::string(_buf.base(), sz);
				}
				return "";
			}

			operator bool() const {
				return _r;
			}

			void reset(io::reader &r) {
				_r = &r;
				_1st_chr_is_cr = false;
				_cache = "";
			}

			void reset() {
				if (!_r) {
					return;
				}
				_r = nullptr;
				_1st_chr_is_cr = false;
				_cache = "";
			}

		private:
			io::reader *_r;
			bool _1st_chr_is_cr;
			bin _buf;
			std::string _cache;
	};

	////////////////////////////////////////////////////////////////////////////

	template <typename T, typename = decltype(std::to_string(std::declval<T>()))>
	inline std::string to_str(T &&src) {
		return std::to_string(std::forward<T>(src));
	}

	inline std::string to_str(std::string src) {
		return src;
	}

	#if RUA_CPP >= RUA_CPP_17 && RUA_HAS_INC(<string_view>)
		inline std::string to_str(std::string_view src) {
			return std::string(src.data(), src.length());
		}
	#endif

	////////////////////////////////////////////////////////////////////////////

	static constexpr uint32_t strjoin_ignore_empty = 0x0000000F;
	static constexpr uint32_t strjoin_multi_line = 0x000000F0;

	inline std::string strjoin(
		const std::vector<std::string> &strs,
		const std::string &sep,
		uint32_t flags = 0,
		size_t reserved_size = 0
	) {
		bool is_ignore_empty = flags & strjoin_ignore_empty;
		bool is_multi_line = flags & strjoin_multi_line;

		size_t len = 0;
		bool bf_is_eol = true;
		for (auto &str : strs) {
			if (!str.length() && is_ignore_empty) {
				continue;
			}
			if (bf_is_eol) {
				bf_is_eol = false;
			} else {
				len += sep.length();
			}
			len += str.length();
			if (is_multi_line && is_eol(str)) {
				bf_is_eol = true;
			}
		}

		std::string r;
		if (!len) {
			return r;
		}
		r.reserve(len + reserved_size);
		r.resize(len);

		size_t pos = 0;
		bf_is_eol = true;
		for (auto &str : strs) {
			if (!str.length() && is_ignore_empty) {
				continue;
			}
			if (bf_is_eol) {
				bf_is_eol = false;
			} else {
				sep.copy(stldata(r) + pos, sep.length());
				pos += sep.length();
			}
			str.copy(stldata(r) + pos, str.length());
			pos += str.length();
			if (is_multi_line && is_eol(str)) {
				bf_is_eol = true;
			}
		}
		return r;
	}
}

#endif
