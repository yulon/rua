#ifndef _RUA_STDIO_HPP
#define _RUA_STDIO_HPP

#include "macros.hpp"

#if defined(_WIN32)

#include "stdio/win32.hpp"

namespace rua {

using namespace win32::_stdio;

} // namespace rua

#elif defined(RUA_UNIX)

#include "stdio/posix.hpp"

namespace rua {

using namespace posix::_stdio;

} // namespace rua

#else

#include "stdio/c.hpp"

namespace rua {

using namespace c::_stdio;

} // namespace rua

#endif

#endif
