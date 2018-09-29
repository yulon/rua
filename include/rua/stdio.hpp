#ifndef _RUA_STDIO_HPP
#define _RUA_STDIO_HPP

#include "io.hpp"
#include "str.hpp"

#if defined(_WIN32)
	#include "windows.h"

	namespace rua {
		inline io::sys::writer stdout_writer() {
			return GetStdHandle(STD_OUTPUT_HANDLE);
		}

		inline io::sys::writer stderr_writer() {
			return GetStdHandle(STD_ERROR_HANDLE);
		}

		inline io::sys::reader stdin_reader() {
			return GetStdHandle(STD_INPUT_HANDLE);
		}
	}
#else
	#include <cstdio>

	namespace rua {
		inline io::sys::writer stdout_writer() {
			return stdout;
		}

		inline io::sys::writer stderr_writer() {
			return stderr;
		}

		inline io::sys::reader stdin_reader() {
			return stdin;
		}
	}
#endif

namespace rua {
	class stdin_line_reader : public line_reader {
		public:
			static stdin_line_reader &instance() {
				static stdin_line_reader inst;
				return inst;
			}

			stdin_line_reader() : _r(stdin_reader()) {
				if (!_r) {
					return;
				}
				#ifdef _WIN32
					_r_tr = _r;
					line_reader::reset(_r_tr);
				#else
					line_reader::reset(_r);
				#endif
			}

		private:
			io::sys::reader _r;

			#ifdef _WIN32
				l_to_u8_reader _r_tr;
			#endif
	};

	inline std::string read_stdin_line() {
		return stdin_line_reader::instance().read_line();
	}
}

#endif
