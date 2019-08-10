#ifndef _RUA_LOG_HPP
#define _RUA_LOG_HPP

#include "bin.hpp"
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

	msgbox_writer(const std::string &title, UINT icon) :
		_tit(u8_to_w(title)),
		_ico(icon) {}

	virtual size_t write(bin_view p) {
		auto sz = p.size();
		for (int i = 0; static_cast<size_t>(i) < p.size(); ++i) {
			if (!is_eol(static_cast<const char &>(p[i]))) {
				continue;
			}
			if (i > 0 || _buf.size()) {
				_buf += std::string(p(0, i).base().to<const char *>(), i);
				MessageBoxW(0, u8_to_w(_buf).c_str(), _tit.c_str(), _ico);
				_buf.resize(0);
			}
			p.slice_self(i + 1);
			i = -1;
		}
		if (p.size()) {
			_buf = std::string(p.base().to<const char *>(), p.size());
		}
		return sz;
	}

private:
	std::wstring _tit;
	UINT _ico;
	std::string _buf;
};

} // namespace win32

#endif

inline printer &log_printer() {
	static auto &p = ([]() -> printer & {
		static printer p;
#ifdef _WIN32
		auto w = get_stdout();
		if (!w) {
			return p;
		}
		p.reset(u8_to_loc_writer(std::move(w)));
#else
		p.reset(get_stdout());
#endif
		return p;
	})();
	return p;
}

template <typename... Args>
inline void log(Args &&... args) {
	log_printer().println(std::forward<Args>(args)...);
}

inline printer &error_log_printer() {
	static auto &p = ([]() -> printer & {
		static printer p;
#ifdef _WIN32
		auto mbw =
			std::make_shared<win32::msgbox_writer>("ERROR", MB_ICONERROR);
		auto w = get_stderr();
		if (!w) {
			p.reset(std::move(mbw));
			return p;
		}
		p.reset(write_group{u8_to_loc_writer(std::move(w)), std::move(mbw)});
#else
		p.reset(get_stderr());
#endif
		return p;
	})();
	return p;
}

template <typename... Args>
inline void error_log(Args &&... args) {
	error_log_printer().println(std::forward<Args>(args)...);
}

} // namespace rua

#endif
