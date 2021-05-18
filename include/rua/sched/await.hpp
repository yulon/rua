#ifndef _RUA_SCHED_AWAIT_HPP
#define _RUA_SCHED_AWAIT_HPP

#include "await/uni.hpp"

#ifdef _WIN32

#include "await/win32.hpp"

namespace rua {

using namespace win32::_wait;

} // namespace rua

#endif

#endif
