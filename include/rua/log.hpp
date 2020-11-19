#ifndef _RUA_LOG_HPP
#define _RUA_LOG_HPP

#include "macros.hpp"
#include "move_only.hpp"
#include "printer.hpp"
#include "stdio.hpp"
#include "string.hpp"
#include "sync.hpp"
#include "thread.hpp"

namespace rua {

#ifdef _WIN32

namespace win32 {

class msgbox_writer : public writer {
public:
	virtual ~msgbox_writer() = default;

	msgbox_writer(string_view default_title, UINT icon) :
		_tit(u8_to_w(default_title)), _ico(icon) {}

	virtual ptrdiff_t write(bytes_view p) {
		auto eol_b = as_bytes(eol::sys_con);
		auto fr = p.find(as_bytes(eol::sys_con));
		if (fr) {
			MessageBoxW(
				0,
				u8_to_w(as_string(p(fr.pos() + eol_b.size()))).c_str(),
				u8_to_w(as_string(p(0, fr.pos()))).c_str(),
				_ico);
		} else {
			MessageBoxW(0, u8_to_w(as_string(p)).c_str(), _tit.c_str(), _ico);
		}
		return static_cast<ptrdiff_t>(p.size());
	}

private:
	std::wstring _tit;
	UINT _ico;
};

} // namespace win32

#endif

inline printer &log_printer() {
	static printer p(sout(), eol::sys_con);
	return p;
}

inline printer &err_log_printer() {
	static printer p(
#ifdef _WIN32
		write_group(
			{serr(),
			 std::make_shared<win32::msgbox_writer>("ERROR", MB_ICONERROR)})
#else
		serr()
#endif
			,
		eol::sys_con);
	return p;
}

inline mutex &log_mutex() {
	static mutex mtx;
	return mtx;
}

template <typename... Args>
inline void log(Args &&... args) {
	auto &p = log_printer();
	if (!p) {
		return;
	}
	auto lg = make_lock_guard(log_mutex());
	p.println(std::forward<Args>(args)...);
}

template <typename... Args>
inline void err_log(Args &&... args) {
	auto &p = err_log_printer();
	if (!p) {
		return;
	}
	auto lg = make_lock_guard(log_mutex());
	p.println(std::forward<Args>(args)...);
}

inline chan<std::function<void()>> &log_chan() {
	static chan<std::function<void()>> ch;
	static thread log_td([]() {
		for (;;) {
			ch.pop()();
		}
	});
	return ch;
}

template <typename... Args>
inline void post_log(Args &&... args) {
	auto &p = log_printer();
	if (!p) {
		return;
	}
	move_only<std::string> s(str_join({to_temp_string(args)...}, " "));
	log_chan() << [&p, s]() mutable {
		auto lg = make_lock_guard(log_mutex());
		p.println(std::move(s.value()));
	};
}

template <typename... Args>
inline void post_err_log(Args &&... args) {
	auto &p = err_log_printer();
	if (!p) {
		return;
	}
	move_only<std::string> s(str_join({to_temp_string(args)...}, " "));
	log_chan() << [&p, s]() mutable {
		auto lg = make_lock_guard(log_mutex());
		p.println(std::move(s.value()));
	};
}

} // namespace rua

#endif
