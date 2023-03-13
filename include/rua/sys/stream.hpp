#ifndef _rua_sys_stream_hpp
#define _rua_sys_stream_hpp

#include "stream/c.hpp"

#include "../util/macros.hpp"

#ifdef _WIN32

#include "stream/win32.hpp"

namespace rua {

using sys_stream = win32::sys_stream;

}

#elif defined(RUA_UNIX)

#include "stream/posix.hpp"

namespace rua {

using sys_stream = posix::sys_stream;

}

#else

namespace rua {

using sys_stream = c_stream;

}

#endif

#endif
