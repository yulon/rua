#ifndef _RUA_SCHED_WAIT_HPP
#define _RUA_SCHED_WAIT_HPP

#include "wait/uni.hpp"

#ifdef _WIN32

#include "wait/win32.hpp"

namespace rua {

using namespace win32::_wait;

} // namespace rua

#endif

#endif
