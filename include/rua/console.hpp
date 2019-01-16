#ifndef _RUA_CONSOLE_HPP
#define _RUA_CONSOLE_HPP

#include "logger.hpp"
#include "stdio.hpp"
#include "str.hpp"
#include "interface_ptr.hpp"

#include <mutex>

namespace rua {

class console : public logger {
public:
	console() = default;

	std::string recv() {
		lock_guard<std::mutex> lg(get_mutex());
		if (!_lr) {
			return "";
		}
		return _lr.read_line();
	}

	void set_recv_reader(io::reader_i rr) {
		lock_guard<std::mutex> lg(get_mutex());

		_lr.reset(std::move(rr));
	}

private:
	line_reader _lr;
};

using console_i = interface_ptr<console>;

inline console_i &default_console() {
	static console_i inst = ([]()->console_i {
		console con;

		auto ow = stdout_writer();
		auto ew = stderr_writer();
		auto ir = stdin_reader();

		#ifdef _WIN32
			con.on_info = [](const std::string &cont) {
				MessageBoxW(0, u8_to_w(cont).c_str(), L"INFORMATION", MB_ICONINFORMATION);
			};
			con.on_warn = [](const std::string &cont) {
				MessageBoxW(0, u8_to_w(cont).c_str(), L"WARNING", MB_ICONWARNING);
			};
			con.on_err = [](const std::string &cont) {
				MessageBoxW(0, u8_to_w(cont).c_str(), L"ERROR", MB_ICONERROR);
			};
			if (!ow) {
				return console_i(std::move(con));
			}
			ow = u8_to_l_writer(ow);
			ew = u8_to_l_writer(ew);
			ir = l_to_u8_reader(ir);
		#endif

		con.set_log_writer(ow);
		con.set_err_writer(ew);
		con.set_recv_reader(ir);

		return console_i(std::move(con));
	})();
	return inst;
}

template <typename... V>
inline void clog(V&&... v) {
	default_console()->log(std::forward<V>(v)...);
}

template <typename... V>
inline void cinfo(V&&... v) {
	default_console()->info(std::forward<V>(v)...);
}

template <typename... V>
inline void cwarn(V&&... v) {
	default_console()->warn(std::forward<V>(v)...);
}

template <typename... V>
inline void cerr(V&&... v) {
	default_console()->err(std::forward<V>(v)...);
}

inline std::string crecv() {
	return default_console()->recv();
}

}

#endif
