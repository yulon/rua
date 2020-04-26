#ifndef _RUA_STRING_LINE_BASE_HPP
#define _RUA_STRING_LINE_BASE_HPP

#include "../view.hpp"

#include "../../macros.hpp"

namespace rua {

namespace eol {

RUA_INLINE_CONST const char *lf = "\n";
RUA_INLINE_CONST const char *crlf = "\r\n";
RUA_INLINE_CONST const char *cr = "\r";

#ifdef _WIN32

RUA_INLINE_CONST const char *sys_text = crlf;
RUA_INLINE_CONST const char *sys_con = crlf;

#elif defined(RUA_DARWIN)

RUA_INLINE_CONST const char *sys_text = cr;
RUA_INLINE_CONST const char *sys_con = lf;

#else

RUA_INLINE_CONST const char *sys_text = lf;
RUA_INLINE_CONST const char *sys_con = lf;

#endif

} // namespace eol

inline bool is_eol(const char c) {
	return c == eol::lf[0] || c == eol::cr[0];
}

inline bool is_eol(string_view str) {
	for (auto &c : str) {
		if (!is_eol(c)) {
			return false;
		}
	}
	return true;
}

} // namespace rua

#endif
