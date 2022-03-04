#ifndef _RUA_HARD_HPP
#define _RUA_HARD_HPP

#include "util/macros.hpp"

#if defined(_WIN32)

#include "hard/win32.hpp"

namespace rua {

using namespace win32::_hard;

} // namespace rua

#elif defined(RUA_UNIX)

#include "hard/posix.hpp"

namespace rua {

using namespace posix::_hard;

} // namespace rua

#else

#error rua::hard: not supported this platform!

#endif

#endif
