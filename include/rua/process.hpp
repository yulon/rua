#ifndef _RUA_PROCESS_HPP
#define _RUA_PROCESS_HPP

#include "macros.hpp"

#if defined(_WIN32)

#include "process/win32.hpp"

namespace rua {

using pid_t = win32::pid_t;
using process = win32::process;
using namespace win32::_this_process;

} // namespace rua

//#elif defined(RUA_UNIX)

//#include "process/unix.hpp"

#else

#error rua::process: not supported this platform!

#endif

#endif
