#ifndef _RUA_SYS_PATHS_HPP
#define _RUA_SYS_PATHS_HPP

#include "../util.hpp"

#if defined(_WIN32)

#include "paths/win32.hpp"

namespace rua {

using namespace win32::_sys_paths;

} // namespace rua

#else

#include "paths/uni.hpp"

namespace rua {

using namespace uni::_sys_paths;

} // namespace rua

#endif

#endif
