#ifndef _RUA_CHRONO_ZONE_WIN32_HPP
#define _RUA_CHRONO_ZONE_WIN32_HPP

#include <windows.h>

#include <cstdlib>
#include <mutex>

namespace rua { namespace win32 {

namespace _chrono_zone {

inline int8_t local_time_zone() {
	TIME_ZONE_INFORMATION info;
	GetTimeZoneInformation(&info);
	return static_cast<int8_t>(info.Bias / (-60));
}

} // namespace _chrono_zone

using namespace _chrono_zone;

}} // namespace rua::win32

#endif
