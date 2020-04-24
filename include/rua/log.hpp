#ifndef _RUA_LOG_HPP
#define _RUA_LOG_HPP

#include "macros.hpp"
#include "printer.hpp"
#include "stdio.hpp"
#include "sync.hpp"

namespace rua {

#ifdef _WIN32

namespace win32 {

class msgbox_writer : public writer {
public:
	virtual ~msgbox_writer() = default;

	msgbox_writer(string_view title, UINT icon) :
		_tit(u8_to_w(title)), _ico(icon) {}

	virtual ptrdiff_t write(bytes_view p) {
		MessageBoxW(0, u8_to_w(p).c_str(), _tit.c_str(), _ico);
		return static_cast<ptrdiff_t>(p.size());
	}

private:
	std::wstring _tit;
	UINT _ico;
};

} // namespace win32

#endif

inline mutex &log_mutex() {
	static mutex mtx;
	return mtx;
}

inline printer &log_printer() {
	static printer p(sout(), eol::sys_con);
	return p;
}

template <typename... Args>
inline void log(Args &&... args) {
	auto lg = make_lock_guard(log_mutex());
	log_printer().println(std::forward<Args>(args)...);
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

template <typename... Args>
inline void err_log(Args &&... args) {
	auto lg = make_lock_guard(log_mutex());
	err_log_printer().println(std::forward<Args>(args)...);
}

} // namespace rua

#endif
