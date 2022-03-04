#ifndef _RUA_PIPE_WIN32_HPP
#define _RUA_PIPE_WIN32_HPP

#include "../string.hpp"
#include "../sys/stream/win32.hpp"
#include "../time/duration.hpp"
#include "../util.hpp"

#include <windows.h>

#include <cassert>

namespace rua { namespace win32 {

namespace _pipe {

struct pipe {
	sys_stream reader, writer;

	operator bool() const {
		return reader || writer;
	}
};

inline bool make_pipe(sys_stream &reader, sys_stream &writer) {
	SECURITY_ATTRIBUTES sa;
	memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);

	HANDLE rh, wh;
	if (!CreatePipe(&rh, &wh, &sa, 0)) {
		return false;
	}
	reader = rh;
	writer = wh;
	return true;
}

inline pipe make_pipe() {
	pipe pa;
	make_pipe(pa.reader, pa.writer);
	return pa;
}

} // namespace _pipe

using namespace _pipe;

}} // namespace rua::win32

#endif
