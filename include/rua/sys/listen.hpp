#ifndef _RUA_SYS_LISTEN_HPP
#define _RUA_SYS_LISTEN_HPP

#include "../util/macros.hpp"

#ifdef _WIN32

#include "listen/win32.hpp"

namespace rua {

using namespace win32::_sys_listen;

} // namespace rua

#endif

#endif
