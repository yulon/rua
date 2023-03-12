#ifndef _RUA_LOG_HPP
#define _RUA_LOG_HPP

#include "io/stream.hpp"
#include "move_only.hpp"
#include "printer.hpp"
#include "stdio.hpp"
#include "string/char_set.hpp"
#include "string/codec.hpp"
#include "string/conv.hpp"
#include "string/join.hpp"
#include "string/view.hpp"
#include "sync.hpp"
#include "thread.hpp"
#include "util.hpp"

namespace rua {

#ifdef _WIN32

namespace win32 {

class msgbox_writer : public stream_base {
public:
	msgbox_writer(string_view default_title, UINT icon) :
		_tit(u2w(default_title)), _ico(icon) {}

	virtual ~msgbox_writer() = default;

	virtual ssize_t write(bytes_view p) {
		auto eol_b = as_bytes(eol::sys);
		auto fr = p.find(as_bytes(eol::sys));
		if (fr) {
			MessageBoxW(
				0,
				u2w(as_string(p(fr.pos() + eol_b.size()))).c_str(),
				u2w(as_string(p(0, fr.pos()))).c_str(),
				_ico);
		} else {
			MessageBoxW(0, u2w(as_string(p)).c_str(), _tit.c_str(), _ico);
		}
		return to_signed(p.size());
	}

private:
	std::wstring _tit;
	UINT _ico;
};

} // namespace win32

#endif

inline printer &log_printer() {
	static printer p(sout(), eol::sys);
	return p;
}

#ifdef _WIN32

inline printer &err_log_printer() {
	static printer p(([]() -> write_group {
		write_group wg({serr(), win32::msgbox_writer("ERROR", MB_ICONERROR)});
		return wg;
	})());
	return p;
}

#else

inline printer &err_log_printer() {
	static printer p(serr(), eol::sys);
	return p;
}

#endif

inline mutex &log_mutex() {
	static mutex mtx;
	return mtx;
}

template <typename... Args>
inline void log(Args &&...args) {
	auto &p = log_printer();
	if (!p) {
		return;
	}
	auto lg = *make_lock_guard(log_mutex());
	p.println(std::forward<Args>(args)...);
}

template <typename... Args>
inline void err_log(Args &&...args) {
	auto &p = err_log_printer();
	if (!p) {
		return;
	}
	auto lg = *make_lock_guard(log_mutex());
	p.println(std::forward<Args>(args)...);
}

inline chan<std::function<void()>> &log_chan() {
	static chan<std::function<void()>> ch;
	static thread log_td([]() {
		for (;;) {
			(**ch.recv())();
		}
	});
	return ch;
}

template <typename... Args>
inline void post_log(Args &&...args) {
	auto &p = log_printer();
	if (!p) {
		return;
	}
	auto s = make_move_only(join({to_temp_string(args)...}, " "));
	log_chan().send([&p, s]() mutable {
		auto lg = *make_lock_guard(log_mutex());
		p.println(std::move(s.value()));
	});
}

template <typename... Args>
inline void post_err_log(Args &&...args) {
	auto &p = err_log_printer();
	if (!p) {
		return;
	}
	auto s = make_move_only(join({to_temp_string(args)...}, " "));
	log_chan().send([&p, s]() mutable {
		auto lg = *make_lock_guard(log_mutex());
		p.println(std::move(s.value()));
	});
}

} // namespace rua

#endif
