#ifndef _rua_sys_listen_hpp
#define _rua_sys_listen_hpp

#include "../util/macros.hpp"

#ifdef _WIN32

#include "listen/win32.hpp"

namespace rua {

using namespace win32::_sys_listen;

} // namespace rua

#endif

#endif
