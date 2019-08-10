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

inline printer make_default_log_printer() {
#ifdef _WIN32
	auto &w = get_stdout();
	if (!w) {
		return nullptr;
	}
	return printer(u8_to_loc_writer(w));
#else
	return printer(get_stdout());
#endif
}

inline printer &log_printer() {
	static auto inst = make_default_log_printer();
	return inst;
}

inline void reset_log_printer() {
	log_printer() = make_default_log_printer();
}

template <typename... Args>
inline void log(Args &&... args) {
	log_printer().println(std::forward<Args>(args)...);
}

inline printer make_default_error_log_printer() {
#ifdef _WIN32
	auto mbw = std::make_shared<win32::msgbox_writer>("ERROR", MB_ICONERROR);
	auto &w = get_stderr();
	if (!w) {
		return printer(std::move(mbw));
	}
	return printer(write_group{u8_to_loc_writer(w), std::move(mbw)});
#else
	return printer(get_stderr());
#endif
}

inline printer &error_log_printer() {
	static auto inst = make_default_error_log_printer();
	return inst;
}

template <typename... Args>
inline void error_log(Args &&... args) {
	error_log_printer().println(std::forward<Args>(args)...);
}

inline void reset_error_log_printer() {
	error_log_printer() = make_default_error_log_printer();
}

} // namespace rua

#endif
