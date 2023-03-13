#ifndef _rua_time_now_hpp
#define _rua_time_now_hpp

#include "../util/macros.hpp"

#ifdef _WIN32

#include "now/win32.hpp"

namespace rua {

using namespace win32::_now;

} // namespace rua

#elif defined(RUA_UNIX)

#include "now/posix.hpp"

namespace rua {

using namespace posix::_now;

} // namespace rua

#else

#error rua::now(): not supported this platform!

#endif

#endif
