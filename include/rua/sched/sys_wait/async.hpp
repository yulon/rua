#ifndef _RUA_SCHED_SYS_WAIT_ASYNC_HPP
#define _RUA_SCHED_SYS_WAIT_ASYNC_HPP

#ifdef _WIN32

#include "async/win32.hpp"

namespace rua {
using namespace win32::_sys_wait_async;
}

#endif

#endif
