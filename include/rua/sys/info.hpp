#ifndef _rua_sys_info_hpp
#define _rua_sys_info_hpp

#include "../util/macros.hpp"

#ifdef _WIN32

#include "info/win32.hpp"

namespace rua {

using namespace win32::_sys_version;

} // namespace rua

#elif defined(RUA_UNIX)

#include "info/posix.hpp"

namespace rua {

using namespace posix::_sys_version;

} // namespace rua

#endif

#endif
