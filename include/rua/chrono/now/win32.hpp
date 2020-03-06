#ifndef _RUA_CHRONO_NOW_WIN32_HPP
#define _RUA_CHRONO_NOW_WIN32_HPP

#include "../time.hpp"

#include "../../dylib/win32.hpp"

#include <windows.h>

#include <cstdint>

namespace rua { namespace win32 {

namespace _now {

static const date_t sys_epoch{1601, 1, 1, 0, 0, 0, 0, 0};

inline time now() {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return time(
		duration_unit<100>(
			static_cast<int64_t>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime),
		sys_epoch);
}

inline int8_t local_time_zone() {
	TIME_ZONE_INFORMATION info;
	GetTimeZoneInformation(&info);
	return static_cast<int8_t>(info.Bias / (-60));
}

inline time local_now() {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return time(
		duration_unit<100>(
			static_cast<int64_t>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime),
		local_time_zone(),
		sys_epoch);
}

} // namespace _now

using namespace _now;

}} // namespace rua::win32

#endif
