#ifndef _rua_sys_wait_hpp
#define _rua_sys_wait_hpp

#include "../util/macros.hpp"

#ifdef _WIN32

#include "wait/win32.hpp"

namespace rua {

using namespace win32::_sys_wait;

} // namespace rua

#endif

#endif
