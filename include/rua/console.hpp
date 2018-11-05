#ifndef _RUA_CONSOLE_HPP
#define _RUA_CONSOLE_HPP

#include "logger.hpp"
#include "stdio.hpp"
#include "str.hpp"

namespace rua {
	class console : public logger {
		public:
			console() = default;

			std::string recv() {
				lock_guard<std::mutex> lg(_mtx);
				if (!_lr) {
					return "";
				}
				return _lr.read_line();
			}

			void set_recv_reader(io::reader &rr) {
				lock_guard<std::mutex> lg(_mtx);

				_lr.reset(rr);
			}

		private:
			line_reader _lr;
	};

	class default_console : public rua::console {
		public:
			static default_console &instance() {
				static default_console inst;
				return inst;
			}

			default_console() : _ow(stdout_writer()), _ew(stderr_writer()), _ir(stdin_reader()) {
				#ifdef _WIN32
					on_info = [](const std::string &cont) {
						MessageBoxW(0, u8_to_w(cont).c_str(), L"INFORMATION", MB_ICONINFORMATION);
					};
					on_warn = [](const std::string &cont) {
						MessageBoxW(0, u8_to_w(cont).c_str(), L"WARNING", MB_ICONWARNING);
					};
					on_err = [](const std::string &cont) {
						MessageBoxW(0, u8_to_w(cont).c_str(), L"ERROR", MB_ICONERROR);
					};
					if (!_ow) {
						return;
					}
					_ow_tr = _ow;
					set_log_writer(_ow_tr);
					_ew_tr = _ew;
					set_err_writer(_ew_tr);
					_ir_tr = _ir;
					set_recv_reader(_ir_tr);
				#else
					set_log_writer(_ow);
					set_err_writer(_ew);
				#endif
			}

		private:
			io::sys::writer _ow, _ew;
			io::sys::reader _ir;

			#ifdef _WIN32
				u8_to_l_writer _ow_tr, _ew_tr;
				l_to_u8_reader _ir_tr;
			#endif
	};

	template <typename... V>
	inline void clog(V&&... v) {
		default_console::instance().log(std::forward<V>(v)...);
	}

	template <typename... V>
	inline void cinfo(V&&... v) {
		default_console::instance().info(std::forward<V>(v)...);
	}

	template <typename... V>
	inline void cwarn(V&&... v) {
		default_console::instance().warn(std::forward<V>(v)...);
	}

	template <typename... V>
	inline void cerr(V&&... v) {
		default_console::instance().err(std::forward<V>(v)...);
	}

	inline std::string crecv() {
		return default_console::instance().recv();
	}
}

#endif
