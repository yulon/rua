#ifndef _RUA_LOGGER_HPP
#define _RUA_LOGGER_HPP

#include "io.hpp"
#include "sched.hpp"
#include "string/strjoin.hpp"
#include "string/to_string.hpp"
#include "sync/mutex.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <vector>

namespace rua {

class logger {
public:
	logger() : _mtx(new mutex), _lw(nullptr), _ew(nullptr), _oe(eol) {}

	template <typename... V>
	void log(V &&... v) {
		_write(_lw, on_log, log_prefix, std::forward<V>(v)...);
	}

	std::string log_prefix;
	std::function<void(const std::string &)> on_log;

	template <typename... V>
	void dbg(V &&... v) {
#ifndef NDEBUG
		_write(_lw, on_dbg, dbg_prefix, std::forward<V>(v)...);
#endif
	}

	std::string dbg_prefix;
	std::function<void(const std::string &)> on_dbg;

	template <typename... V>
	void info(V &&... v) {
		_write(_lw, on_info, info_prefix, std::forward<V>(v)...);
	}

	std::string info_prefix;
	std::function<void(const std::string &)> on_info;

	template <typename... V>
	void warn(V &&... v) {
		_write(_lw, on_warn, warn_prefix, std::forward<V>(v)...);
	}

	std::string warn_prefix;
	std::function<void(const std::string &)> on_warn;

	template <typename... V>
	void err(V &&... v) {
		_write(_ew, on_err, err_prefix, std::forward<V>(v)...);
	}

	std::string err_prefix;
	std::function<void(const std::string &)> on_err;

	void set_writer(writer_i w) {
		std::lock_guard<mutex> lg(*_mtx);

		_lw = w;
		_ew = std::move(w);
	}

	void set_log_writer(writer_i lw) {
		std::lock_guard<mutex> lg(*_mtx);

		_lw = std::move(lw);
	}

	writer_i get_log_writer() {
		return _lw;
	}

	void set_err_writer(writer_i ew) {
		std::lock_guard<mutex> lg(*_mtx);

		_ew = std::move(ew);
	}

	writer_i get_err_writer() {
		return _ew;
	}

	void set_over_mark(const char *str = eol) {
		std::lock_guard<mutex> lg(*_mtx);

		_oe = str;
	}

	mutex &get_mutex() {
		return *_mtx;
	}

	void reset() {
		std::lock_guard<mutex> lg(*_mtx);

		_lw.reset();
		_ew.reset();
	}

protected:
	std::unique_ptr<mutex> _mtx;

private:
	writer_i _lw, _ew;
	const char *_oe;

	template <typename... V>
	void _write(
		const writer_i &w,
		std::function<void(const std::string &)> &on,
		const std::string &prefix,
		V &&... v) {
		std::vector<std::string> strs;
		if (prefix.length()) {
			strs = {prefix, to_string(v)..., eol};
		} else {
			strs = {to_string(v)..., eol};
		}

		if (on) {
			auto cont = strjoin(strs, " ", strjoin_multi_line);
			on(cont);

			std::lock_guard<mutex> lg(*_mtx);
			if (!w) {
				return;
			}
			w->write_all(cont);
			return;
		}

		std::lock_guard<mutex> lg(*_mtx);
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

} // namespace rua

#endif
