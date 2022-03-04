#ifndef _RUA_DYLIB_HPP
#define _RUA_DYLIB_HPP

#include "util/macros.hpp"

#ifdef _WIN32

#include "dylib/win32.hpp"

namespace rua {

using dylib = win32::dylib;

} // namespace rua

#elif defined(RUA_UNIX) || RUA_HAS_INC(<dlfcn.h>)

#include "dylib/posix.hpp"

namespace rua {

using dylib = posix::dylib;

} // namespace rua

#else

#error rua::dylib: not supported this platform!

#endif

#endif
