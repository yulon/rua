#ifndef _RUA_CHRONO_CLOCK_HPP
#define _RUA_CHRONO_CLOCK_HPP

#include "../macros.hpp"

#ifdef _WIN32

#include "clock/win32.hpp"

namespace rua {

using namespace win32::_clock;

} // namespace rua

#elif defined(RUA_DARWIN)

#include "clock/darwin.hpp"

namespace rua {

using namespace darwin::_clock;

} // namespace rua

#elif defined(RUA_UNIX)

#include "clock/posix.hpp"

namespace rua {

using namespace posix::_clock;

} // namespace rua

#else

#error rua::clock: not supported this platform!

#endif

#endif
