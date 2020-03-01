#ifndef _RUA_IO_SYS_STREAM_HPP
#define _RUA_IO_SYS_STREAM_HPP

#include "../macros.hpp"

#ifdef _WIN32

#include "sys_stream/win32.hpp"
namespace rua {
using sys_stream = win32::sys_stream;
}

#elif defined(RUA_UNIX)

#include "sys_stream/posix.hpp"
namespace rua {
using sys_stream = posix::sys_stream;
}

#else

#include "c_stream.hpp"
namespace rua {
using sys_stream = c_stream;
}

#endif

#endif
