#ifndef _rua_stdio_hpp
#define _rua_stdio_hpp

#include "stdio/c.hpp"

#include "util/macros.hpp"

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

namespace rua {

using namespace c::_stdio;

} // namespace rua

#endif

#endif
