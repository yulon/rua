#ifndef _RUA_LOGGER_HPP
#define _RUA_LOGGER_HPP

#include "io.hpp"
#include "str.hpp"
#include "sched.hpp"

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
			logger() : _lw(nullptr), _ew(nullptr), _oe(eol) {}

			template <typename... V>
			void log(V&&... v) {
				_write(_lw, on_log, log_prefix, std::forward<V>(v)...);
			}

			std::string log_prefix;
			std::function<void(const std::string &)> on_log;

			template <typename... V>
			void dbg(V&&... v) {
				#ifndef NDEBUG
					_write(_lw, on_dbg, dbg_prefix, std::forward<V>(v)...);
				#endif
			}

			std::string dbg_prefix;
			std::function<void(const std::string &)> on_dbg;

			template <typename... V>
			void info(V&&... v) {
				_write(_lw, on_info, info_prefix, std::forward<V>(v)...);
			}

			std::string info_prefix;
			std::function<void(const std::string &)> on_info;

			template <typename... V>
			void warn(V&&... v) {
				_write(_lw, on_warn, warn_prefix, std::forward<V>(v)...);
			}

			std::string warn_prefix;
			std::function<void(const std::string &)> on_warn;

			template <typename... V>
			void err(V&&... v) {
				_write(_ew, on_err, err_prefix, std::forward<V>(v)...);
			}

			std::string err_prefix;
			std::function<void(const std::string &)> on_err;

			void set_writer(io::writer &w) {
				lock_guard<std::mutex> lg(_mtx);

				_lw = &w;
				_ew = &w;
			}

			void set_log_writer(io::writer &lw) {
				lock_guard<std::mutex> lg(_mtx);

				_lw = &lw;
			}

			void set_err_writer(io::writer &ew) {
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

		protected:
			std::mutex _mtx;

		private:
			io::writer *_lw, *_ew;
			const char *_oe;

			template <typename... V>
			void _write(io::writer *w, std::function<void(const std::string &)> &on, const std::string &prefix, V&&... v) {
				std::vector<std::string> strs;
				if (prefix.length()) {
					strs = { prefix, to_str(v)..., eol };
				} else {
					strs = { to_str(v)..., eol };
				}

				if (on) {
					auto cont = strjoin(strs, " ", strjoin_multi_line);
					on(cont);

					lock_guard<std::mutex> lg(_mtx);
					if (!w) {
						return;
					}
					w->write_all(cont);
					return;
				}

				lock_guard<std::mutex> lg(_mtx);
				if (!w) {
					return;
				}
				for (auto &str : strs) {
					w->write_all(str);
					if (&str != &strs.back()) {
						w->write_all(" ");
					}
				}
			}
	};
}

#endif
