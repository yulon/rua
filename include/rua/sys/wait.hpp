#ifndef _RUA_SYS_WAIT_HPP
#define _RUA_SYS_WAIT_HPP

#include "../util/macros.hpp"

#ifdef _WIN32

#include "wait/win32.hpp"

namespace rua {

using namespace win32::_sys_wait;

} // namespace rua

#endif

#endif
