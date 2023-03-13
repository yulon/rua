#ifndef _rua_sys_paths_hpp
#define _rua_sys_paths_hpp

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
