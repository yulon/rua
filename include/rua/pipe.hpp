#ifndef _rua_pipe_hpp
#define _rua_pipe_hpp

#include "util/macros.hpp"

#if defined(_WIN32)

#include "pipe/win32.hpp"

namespace rua {

using namespace win32::_pipe;

} // namespace rua

#else

#error rua::pipe: not supported this platform!

#endif

#endif
