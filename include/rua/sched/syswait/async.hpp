#ifndef _RUA_SCHED_SYSWAIT_ASYNC_HPP
#define _RUA_SCHED_SYSWAIT_ASYNC_HPP

#ifdef _WIN32

#include "async/win32.hpp"

namespace rua {
using namespace win32::_syswait_async;
}

#endif

#endif
