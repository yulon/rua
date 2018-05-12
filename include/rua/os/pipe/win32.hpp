#ifndef _RUA_OS_PIPE_WIN32_HPP
#define _RUA_OS_PIPE_WIN32_HPP

#ifndef _WIN32
	#error rua::os::pipe: not supported this platform!
#endif

#include "../../strenc.hpp"

#include <windows.h>

#include <streambuf>
#include <ios>
#include <string>
#include <cassert>

namespace rua {
	namespace os {
		template<typename CharT, typename Traits = std::char_traits<CharT>>
		class basic_pipebuf : public std::basic_streambuf<CharT, Traits> {
			public:
				using native_handle_t = HANDLE;

				basic_pipebuf() : _h(nullptr) {}

				basic_pipebuf(
					const std::string &name,
					std::ios_base::openmode which = std::ios_base::in | std::ios_base::out | std::ios_base::trunc,
					DWORD timeout = NMPWAIT_WAIT_FOREVER
				) {
					_open(name, which, timeout);
				}

				virtual ~basic_pipebuf() {
					close();
				}

				bool open(
					const std::string &name,
					std::ios_base::openmode which = std::ios_base::in | std::ios_base::out | std::ios_base::trunc,
					DWORD timeout = NMPWAIT_WAIT_FOREVER
				) {
					close();
					return _open(name, which, timeout);
				}

				bool week_open(
					const std::string &name,
					DWORD timeout = NMPWAIT_WAIT_FOREVER
				) {
					close();
					return _open(name, std::ios_base::in | std::ios_base::out, timeout);
				}

				native_handle_t native_handle() const {
					return _h;
				}

				bool is_open() const {
					return _h;
				}

				operator bool() const {
					return is_open();
				}

				void close() {
					if (!_h) {
						return;
					}
					if (_is_svr) {
						DisconnectNamedPipe(_h);
					}
					CloseHandle(_h);
					_h = nullptr;
				}

			protected:
				virtual int underflow() {
					return _h ? 0 : Traits::eof();
				}

				virtual int uflow() {
					CharT c;
					DWORD w_sz;
					if (!ReadFile(_h, &c, sizeof(CharT), &w_sz, nullptr) || w_sz != sizeof(CharT)) {
						close();
						return Traits::eof();
					}
					return c;
				}

				virtual std::streamsize xsgetn(CharT *s, std::streamsize size) {
					auto need_sz = size;
					DWORD w_sz;
					do {
						if (!ReadFile(_h, s, size, &w_sz, nullptr)) {
							close();
							return size - need_sz;
						}
						need_sz -= w_sz;
					} while (need_sz);
					return size;
				}

				virtual int overflow(int ch = Traits::eof()) {
					if (ch == Traits::eof()) {
						close();
						return Traits::eof();
					}
					CharT c = ch;
					DWORD w_sz;
					if (!WriteFile(_h, &c, sizeof(CharT), &w_sz, nullptr) || w_sz != sizeof(CharT)) {
						close();
						return Traits::eof();
					}
					return c;
				}

				virtual std::streamsize xsputn(const CharT *s, std::streamsize size) {
					auto need_sz = size;
					DWORD w_sz;
					do {
						if (!WriteFile(_h, s, size, &w_sz, nullptr)) {
							close();
							return size - need_sz;
						}
						need_sz -= w_sz;
					} while (need_sz);
					return size;
				}

			private:
				native_handle_t _h;
				bool _is_svr;

				bool _open(
					const std::string &name,
					std::ios_base::openmode which,
					DWORD timeout
				) {
					auto fmt_name = rua::u8_to_w("\\\\.\\pipe\\" + name);

					if (WaitNamedPipeW(fmt_name.c_str(), timeout)) {
						DWORD om = 0;
						if ((which & std::ios_base::in) == std::ios_base::in) {
							om |= GENERIC_READ;
						}
						if ((which & std::ios_base::out) == std::ios_base::out) {
							om |= GENERIC_WRITE;
						}
						_h = CreateFileW(
							fmt_name.c_str(),
							om,
							0,
							nullptr,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							nullptr
						);
						if (!_h) {
							return false;
						}
						_is_svr = false;
						return true;
					}

					if ((which & std::ios_base::trunc) != std::ios_base::trunc) {
						_h = nullptr;
						return false;
					}

					DWORD om = 0;
					if ((which & std::ios_base::in) == std::ios_base::in) {
						om |= PIPE_ACCESS_INBOUND;
					}
					if ((which & std::ios_base::out) == std::ios_base::out) {
						om |= PIPE_ACCESS_OUTBOUND;
					}
					_h = CreateNamedPipeW(
						fmt_name.c_str(),
						om,
						PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
						PIPE_UNLIMITED_INSTANCES,
						0,
						0,
						timeout,
						0
					);
					if (!_h) {
						return false;
					}
					if (!ConnectNamedPipe(_h, nullptr)) {
						return false;
					}
					_is_svr = true;
					return true;
				}
		};

		using pipebuf = basic_pipebuf<char>;
		using wpipebuf = basic_pipebuf<wchar_t>;

		template<typename CharT, typename Traits = std::char_traits<CharT>>
		class basic_pipestream : public std::basic_iostream<CharT, Traits> {
			public:
				using native_handle_t = typename basic_pipebuf<CharT, Traits>::native_handle_t;

				basic_pipestream() : std::basic_iostream<CharT, Traits>(&_buf) {}

				basic_pipestream(
					const std::string &name,
					std::ios_base::openmode which = std::ios_base::in | std::ios_base::out | std::ios_base::trunc,
					DWORD timeout = NMPWAIT_WAIT_FOREVER
				) : std::basic_iostream<CharT, Traits>(&_buf), _buf(name, which, timeout) {}

				virtual ~basic_pipestream() = default;

				bool open(
					const std::string &name,
					std::ios_base::openmode which = std::ios_base::in | std::ios_base::out | std::ios_base::trunc,
					DWORD timeout = NMPWAIT_WAIT_FOREVER
				) {
					return _buf.open(name, which, timeout);
				}

				bool week_open(
					const std::string &name,
					DWORD timeout = NMPWAIT_WAIT_FOREVER
				) {
					return _buf.week_open(name, timeout);
				}

				native_handle_t native_handle() const {
					return _buf.native_handle();
				}

				bool is_open() const {
					return _buf.is_open();
				}

				operator bool() const {
					return _buf;
				}

				void close() {
					_buf.close();
				}

			private:
				basic_pipebuf<CharT, Traits> _buf;
		};

		using pipestream = basic_pipestream<char>;
		using wpipestream = basic_pipestream<wchar_t>;
	}
}

#endif
