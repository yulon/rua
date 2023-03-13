#ifndef _rua_time_tick_hpp
#define _rua_time_tick_hpp

#include "../util/macros.hpp"

#ifdef _WIN32

#include "tick/win32.hpp"

namespace rua {

using namespace win32::_tick;

} // namespace rua

#elif defined(RUA_DARWIN)

#include "tick/darwin.hpp"

namespace rua {

using namespace darwin::_tick;

} // namespace rua

#elif defined(RUA_UNIX)

#include "tick/posix.hpp"

namespace rua {

using namespace posix::_tick;

} // namespace rua

#else

#error rua::tick(): not supported this platform!

#endif

#endif
