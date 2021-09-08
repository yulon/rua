#ifndef _RUA_THREAD_WAIT_WIN32_HPP
#define _RUA_THREAD_WAIT_WIN32_HPP

#include "../basic/win32.hpp"

#include "../../sys/wait/win32.hpp"

#include <windows.h>

namespace rua { namespace win32 {

inline any_word thread::wait_for_exit() {
	if (!_h) {
		return 0;
	}
	sys_wait(_h);
	DWORD exit_code;
	GetExitCodeThread(_h, &exit_code);
	reset();
	return static_cast<int>(exit_code);
}

}} // namespace rua::win32

#endif
