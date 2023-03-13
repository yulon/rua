#ifndef _rua_time_now_win32_hpp
#define _rua_time_now_win32_hpp

#include "../real.hpp"

#include "../../dylib/win32.hpp"
#include "../../util.hpp"

#include <windows.h>

namespace rua { namespace win32 {

namespace _now {

inline int8_t local_time_zone() {
	TIME_ZONE_INFORMATION info;
	GetTimeZoneInformation(&info);
	return static_cast<int8_t>(info.Bias / 60 + 16);
}

RUA_CVAR const date sys_epoch{1601, 1, 1, 0, 0, 0, 0, 0};

RUA_CVAL auto _FILETIME_low_bsz = sizeof(FILETIME::dwLowDateTime) * 8;
RUA_CVAL auto _FILETIME_high_bsz = 64 - _FILETIME_low_bsz;

using sys_time_t = FILETIME;

inline time
from_sys_time(const sys_time_t &ft, int8_t zone = local_time_zone()) {
	return time(
		duration::from<100>(
			static_cast<int64_t>(ft.dwHighDateTime) << _FILETIME_low_bsz |
			ft.dwLowDateTime),
		zone,
		sys_epoch);
}

inline sys_time_t to_sys_time(const time &ti) {
	auto num = ti.to(sys_epoch).elapsed().count<100>();
	return {
		static_cast<decltype(FILETIME::dwLowDateTime)>(
			num << _FILETIME_high_bsz >> _FILETIME_high_bsz),
		static_cast<decltype(FILETIME::dwHighDateTime)>(
			num >> _FILETIME_low_bsz)};
}

inline time now(int8_t zone = local_time_zone()) {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return from_sys_time(ft, zone);
}

} // namespace _now

using namespace _now;

}} // namespace rua::win32

#endif
