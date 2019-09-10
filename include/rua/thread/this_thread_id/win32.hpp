#ifndef _RUA_THREAD_THIS_THREAD_ID_WIN32_HPP
#define _RUA_THREAD_THIS_THREAD_ID_WIN32_HPP

#include "../thread_id/win32.hpp"

#include "../../macros.hpp"

#include <windows.h>

namespace rua { namespace win32 {

namespace _this_thread_id {

RUA_FORCE_INLINE thread_id_t this_thread_id() {
	return GetCurrentThreadId();
}

} // namespace _this_thread_id

using namespace _this_thread_id;

}} // namespace rua::win32

#endif
