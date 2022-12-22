#ifndef _RUA_THREAD_ID_WIN32_HPP
#define _RUA_THREAD_ID_WIN32_HPP

#include <windows.h>

namespace rua { namespace win32 {

using tid_t = DWORD;

namespace _this_tid {

inline tid_t this_tid() {
	return GetCurrentThreadId();
}

} // namespace _this_tid

using namespace _this_tid;

}} // namespace rua::win32

#endif
