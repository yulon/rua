#ifndef _RUA_SYS_HANDLE_WAIT_FOR_FINISH_HPP
#define _RUA_SYS_HANDLE_WAIT_FOR_FINISH_HPP

#include "../../macros.hpp"

#ifdef _WIN32

#include "wait_for_finish/win32.hpp"

namespace rua {

using namespace win32::_wait_for_sys_handle_finish;

} // namespace rua

#endif

#endif
