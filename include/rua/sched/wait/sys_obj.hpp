#ifndef _RUA_SCHED_WAIT_SYS_OBJ_HPP
#define _RUA_SCHED_WAIT_SYS_OBJ_HPP

#ifdef _WIN32

#include "sys_obj/win32.hpp"
#include "sys_obj/win32_async.hpp"

namespace rua {

using namespace win32::_wait_sys_obj;
using namespace win32::_wait_sys_obj_async;

} // namespace rua

#endif

#endif
