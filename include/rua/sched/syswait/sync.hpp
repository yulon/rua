#ifndef _RUA_SCHED_SYSWAIT_SYNC_HPP
#define _RUA_SCHED_SYSWAIT_SYNC_HPP

#ifdef _WIN32

#include "sync/win32.hpp"

namespace rua {
using namespace win32::_syswait_sync;
}

#endif

#endif
