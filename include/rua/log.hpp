#ifndef _RUA_LOG_HPP
#define _RUA_LOG_HPP

#include "io.hpp"
#include "str.hpp"
#include "sched.hpp"
#include "stdio.hpp"

#include <mutex>
#include <vector>
#include <type_traits>
#include <sstream>

namespace rua {
	class logger {
		public:
			logger() : _lw(nullptr), _ew(nullptr), _oe(line_end) {}

			logger(io::writer &lw, const char *w_end = line_end) : _lw(&lw), _ew(&lw), _oe(w_end) {}

			logger(io::writer &lw, io::writer &ew, const char *w_end = line_end) : _lw(&lw), _ew(&ew), _oe(w_end) {}

			template <typename... V>
			void log(V&&... v) {
				_write(*_lw, std::forward<V>(v)...);
			}

			template <typename... V>
			void logi(V&&... v) {
				_write(*_lw, std::forward<V>(v)...);
			}

			template <typename... V>
			void logw(V&&... v) {
				_write(*_lw, std::forward<V>(v)...);
			}

			template <typename... V>
			void loge(V&&... v) {
				_write(*_ew, std::forward<V>(v)...);
			}

			void set_log_writer(io::writer &lw) {
				lock_guard<std::mutex> lg(_mtx);

				_lw = &lw;
			}

			void set_error_writer(io::writer &ew) {
				lock_guard<std::mutex> lg(_mtx);

				_ew = &ew;
			}

			void set_output_end(const char *str = line_end) {
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
			void _write(io::writer &w, V&&... v) {
				std::vector<std::string> strs({ to_str<typename std::decay<V>::type>::get(v)... });
				for (auto &str : strs) {
					if (str.empty()) {
						str = "<null>";
					}
				}

				auto line = join(strs, " ", strlen(_oe));
				line += _oe;

				lock_guard<std::mutex> lg(_mtx);
				w.write_all(line);
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
					/*if (!_ow || !_ew) {
						AllocConsole();
						if (!_ow) {
							_ow = stdout_writer();
						}
						if (!_ew) {
							_ew = stderr_writer();
						}
					}*/
					_ow_tr = _ow;
					_ew_tr = _ew;
					set_log_writer(_ow_tr);
					set_error_writer(_ew_tr);
				#else
					set_log_writer(_ow);
					set_error_writer(_ew);
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
