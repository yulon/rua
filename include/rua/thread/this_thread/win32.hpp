#ifndef _RUA_THREAD_THIS_THREAD_WIN32_HPP
#define _RUA_THREAD_THIS_THREAD_WIN32_HPP

#include "../this_thread_id/win32.hpp"
#include "../thread/win32.hpp"

#include "../../macros.hpp"

namespace rua { namespace win32 {

namespace _thread_this_thread {

RUA_FORCE_INLINE thread this_thread() {
	return thread(this_thread_id());
}

} // namespace _thread_this_thread

using namespace _thread_this_thread;

}} // namespace rua::win32

#endif
