#ifndef _RUA_CHRONO_NOW_WIN32_HPP
#define _RUA_CHRONO_NOW_WIN32_HPP

#include "../time.hpp"

#include "../../dylib/win32.hpp"
#include "../../macros.hpp"
#include "../../types/util.hpp"

#include <windows.h>

namespace rua { namespace win32 {

namespace _now {

static const date_t sys_epoch{1601, 1, 1, 0, 0, 0, 0, 0};

RUA_FORCE_INLINE int8_t local_time_zone() {
	TIME_ZONE_INFORMATION info;
	GetTimeZoneInformation(&info);
	return static_cast<int8_t>(info.Bias / 60 + 16);
}

RUA_FORCE_INLINE time now(int8_t zone = local_time_zone()) {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return time(
		duration::from<100>(
			static_cast<int64_t>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime),
		zone,
		sys_epoch);
}

} // namespace _now

using namespace _now;

}} // namespace rua::win32

#endif
