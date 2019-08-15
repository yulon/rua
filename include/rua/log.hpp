#ifndef _RUA_LOG_HPP
#define _RUA_LOG_HPP

#include "bytes.hpp"
#include "io.hpp"
#include "printer.hpp"
#include "stdio.hpp"
#include "string.hpp"

namespace rua {

#ifdef _WIN32

namespace win32 {

class msgbox_writer : public writer {
public:
	virtual ~msgbox_writer() = default;

	msgbox_writer(string_view title, UINT icon) :
		_tit(u8_to_w(title)),
		_ico(icon) {}

	virtual size_t write(bytes_view p) {
		MessageBoxW(
			0,
			u8_to_w(
				std::string(reinterpret_cast<const char *>(p.data()), p.size()))
				.c_str(),
			_tit.c_str(),
			_ico);
		return p.size();
	}

private:
	std::wstring _tit;
	UINT _ico;
};

} // namespace win32

#endif

inline printer &log_printer() {
#ifdef _WIN32
	static auto &p = ([]() -> printer & {
		static printer p;
		auto w = get_stdout();
		if (!w) {
			return p;
		}
		p.reset(u8_to_loc_writer(std::move(w)));
		return p;
	})();
#else
	static printer p(get_stdout(), eol::lf);
#endif
	return p;
}

template <typename... Args>
inline void log(Args &&... args) {
	log_printer().println(std::forward<Args>(args)...);
}

inline printer &error_log_printer() {
#ifdef _WIN32
	static auto &p = ([]() -> printer & {
		static printer p;
		auto mbw =
			std::make_shared<win32::msgbox_writer>("ERROR", MB_ICONERROR);
		auto w = get_stderr();
		if (!w) {
			p.reset(std::move(mbw));
			return p;
		}
		p.reset(write_group{u8_to_loc_writer(std::move(w)), std::move(mbw)});
		return p;
	})();
#else
	static printer p(get_stderr(), eol::lf);
#endif
	return p;
}

template <typename... Args>
inline void error_log(Args &&... args) {
	error_log_printer().println(std::forward<Args>(args)...);
}

} // namespace rua

#endif
