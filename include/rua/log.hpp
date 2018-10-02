#ifndef _RUA_LOG_HPP
#define _RUA_LOG_HPP

#include "io.hpp"
#include "str.hpp"
#include "sched.hpp"
#include "stdio.hpp"

#ifdef _WIN32
	#include <windows.h>
#endif

#include <mutex>
#include <vector>
#include <type_traits>
#include <functional>

namespace rua {
	class logger {
		public:
			logger(std::function<void(logger &)> init = nullptr) : _lw(nullptr), _ew(nullptr), _oe(eol) {
				if (init) {
					init(*this);
				}
			}

			template <typename... V>
			void log(V&&... v) {
				_write(_lw, on_log, log_prefix, std::forward<V>(v)...);
			}

			std::string log_prefix;
			std::function<void(const std::string &)> on_log;

			template <typename... V>
			void logi(V&&... v) {
				_write(_lw, on_logi, logi_prefix, std::forward<V>(v)...);
			}

			std::string logi_prefix;
			std::function<void(const std::string &)> on_logi;

			template <typename... V>
			void logw(V&&... v) {
				_write(_lw, on_logw, logw_prefix, std::forward<V>(v)...);
			}

			std::string logw_prefix;
			std::function<void(const std::string &)> on_logw;

			template <typename... V>
			void loge(V&&... v) {
				_write(_ew, on_loge, loge_prefix, std::forward<V>(v)...);
			}

			std::string loge_prefix;
			std::function<void(const std::string &)> on_loge;

			void set_writer(io::writer &w) {
				lock_guard<std::mutex> lg(_mtx);

				_lw = &w;
				_ew = &w;
			}

			void set_log_writer(io::writer &lw) {
				lock_guard<std::mutex> lg(_mtx);

				_lw = &lw;
			}

			void set_loge_writer(io::writer &ew) {
				lock_guard<std::mutex> lg(_mtx);

				_ew = &ew;
			}

			void set_over_mark(const char *str = eol) {
				lock_guard<std::mutex> lg(_mtx);

				_oe = str;
			}

			void reset() {
				lock_guard<std::mutex> lg(_mtx);

				_lw = nullptr;
				_ew = nullptr;
			}

		private:
			std::mutex _mtx;
			io::writer *_lw, *_ew;
			const char *_oe;

			template <typename... V>
			void _write(io::writer *w, std::function<void(const std::string &)> &on, const std::string &prefix, V&&... v) {
				std::vector<std::string> strs;
				if (prefix.length()) {
					strs = { prefix, to_str<typename std::decay<V>::type>::get(v)..., eol };
				} else {
					strs = { to_str<typename std::decay<V>::type>::get(v)..., eol };
				}

				for (auto &str : strs) {
					if (str.empty()) {
						str = "<null>";
					}
				}

				auto cont = strjoin(strs, " ", strjoin_multi_line);

				if (on) {
					on(cont);
				}

				if (!w) {
					return;
				}
				lock_guard<std::mutex> lg(_mtx);
				w->write_all(cont);
			}
	};

	////////////////////////////////////////////////////////////////////////////

	class default_logger : public logger {
		public:
			static default_logger &instance() {
				static default_logger inst;
				return inst;
			}

		private:
			default_logger() : _ow(stdout_writer()), _ew(stderr_writer()) {
				#ifdef _WIN32
					if (!_ow) {
						on_log = [](const std::string &cont) {
							MessageBoxW(0, u8_to_w(cont).c_str(), L"LOG", MB_OK);
						};
						on_logi = [](const std::string &cont) {
							MessageBoxW(0, u8_to_w(cont).c_str(), L"INFORMATION", MB_ICONINFORMATION);
						};
						on_logw = [](const std::string &cont) {
							MessageBoxW(0, u8_to_w(cont).c_str(), L"WARNING", MB_ICONWARNING);
						};
						on_loge = [](const std::string &cont) {
							MessageBoxW(0, u8_to_w(cont).c_str(), L"ERROR", MB_ICONERROR);
						};
						return;
					}
					_ow_tr = _ow;
					set_log_writer(_ow_tr);
					_ew_tr = _ew;
					set_loge_writer(_ew_tr);
				#else
					set_log_writer(_ow);
					set_loge_writer(_ew);
				#endif
			}

			io::sys::writer _ow, _ew;

			#ifdef _WIN32
				u8_to_l_writer _ow_tr, _ew_tr;
			#endif
	};

	template <typename... V>
	inline void log(V&&... v) {
		default_logger::instance().log(std::forward<V>(v)...);
	}

	template <typename... V>
	inline void logi(V&&... v) {
		default_logger::instance().logi(std::forward<V>(v)...);
	}

	template <typename... V>
	inline void logw(V&&... v) {
		default_logger::instance().logw(std::forward<V>(v)...);
	}

	template <typename... V>
	inline void loge(V&&... v) {
		default_logger::instance().loge(std::forward<V>(v)...);
	}
}

#endif
