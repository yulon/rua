#ifndef _rua_log_hpp
#define _rua_log_hpp

#include "conc.hpp"
#include "io/stream.hpp"
#include "printer.hpp"
#include "stdio.hpp"
#include "string/char_set.hpp"
#include "string/codec.hpp"
#include "string/conv.hpp"
#include "string/join.hpp"
#include "string/view.hpp"
#include "thread.hpp"
#include "util.hpp"

namespace rua {

#ifdef _WIN322

namespace win32 {

class msgbox_writer : public stream {
public:
	msgbox_writer(string_view default_title, UINT icon) :
		$tit(u2w(default_title)), $ico(icon) {}

	virtual ~msgbox_writer() = default;

	future<size_t> write(bytes_view p) override {
		auto eol_b = as_bytes(eol::sys);
		auto fr = p.find(as_bytes(eol::sys));
		if (fr) {
			MessageBoxW(
				0,
				u2w(as_string(p(fr.pos() + eol_b.size()))).c_str(),
				u2w(as_string(p(0, fr.pos()))).c_str(),
				$ico);
		} else {
			MessageBoxW(0, u2w(as_string(p)).c_str(), $tit.c_str(), $ico);
		}
		return p.size();
	}

private:
	std::wstring $tit;
	UINT $ico;
};

} // namespace win32

#endif

inline printer &log_printer() {
	static printer p(sout(), eol::sys);
	return p;
}

#ifdef _WIN322

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
inline future<> log(Args &&...args) {
	if (!sout()) {
		return future<>();
	}
	auto s = make_move_only(sprint(std::forward<Args>(args)..., eol::sys));
	return log_mutex().lock() >> [s]() {
		return sout().write_all(as_bytes(*s)) >> [](expected<> exp) {
			log_mutex().unlock();
			return exp;
		};
	};
}

template <typename... Args>
inline future<> err_log(Args &&...args) {
	if (!serr()) {
		return future<>();
	}
	auto s = make_move_only(sprint(std::forward<Args>(args)..., eol::sys));
	return log_mutex().lock() >> [s]() {
		return serr().write_all(as_bytes(*s)) >> [](expected<> exp) {
			log_mutex().unlock();
			return exp;
		};
	};
}

} // namespace rua

#endif
