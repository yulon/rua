#ifndef _RUA_CHRONO_ZONE_HPP
#define _RUA_CHRONO_ZONE_HPP

#include "../macros.hpp"

#ifdef _WIN32

#include "zone/win32.hpp"

namespace rua {

using namespace win32::_zone;

} // namespace rua

#elif defined(RUA_UNIX)

#include "zone/posix.hpp"

namespace rua {

using namespace posix::_zone;

} // namespace rua

#else

#error rua::local_time_zone: not supported this platform!

#endif

#endif
