#ifndef _RUA_CHRONO_ZONE_HPP
#define _RUA_CHRONO_ZONE_HPP

#include "../macros.hpp"

#ifdef _WIN32

#include "zone/win32.hpp"

namespace rua {

RUA_FORCE_INLINE int8_t local_time_zone() {
	return win32::local_time_zone();
}

} // namespace rua

#elif defined(RUA_UNIX)

#include "zone/posix.hpp"

namespace rua {

RUA_FORCE_INLINE int8_t local_time_zone() {
	return posix::local_time_zone();
}

} // namespace rua

#else

#error rua::local_time_zone: not supported this platform!

#endif

#endif
