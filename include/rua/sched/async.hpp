#ifndef _RUA_SCHED_ASYNC_HPP
#define _RUA_SCHED_ASYNC_HPP

#include "async/uni.hpp"

#ifdef _WIN32

#include "async/win32.hpp"

namespace rua {

using namespace win32::_async;

} // namespace rua

#endif

#endif
