#ifndef _RUA_TIME_NOW_POSIX_HPP
#define _RUA_TIME_NOW_POSIX_HPP

#include "../real.hpp"

#include "../../util.hpp"

#include <sys/time.h>
#include <time.h>

namespace rua { namespace posix {

namespace _now {

inline int8_t local_time_zone() {
	tzset();
	return static_cast<int8_t>(timezone / 3600 + 16);
}

RUA_CVAR const auto sys_epoch = unix_epoch;

using sys_time_t = struct timeval;

inline time
from_sys_time(const sys_time_t &tv, int8_t zone = local_time_zone()) {
	return time(duration(tv.tv_sec, tv.tv_usec * 1000), zone, unix_epoch);
}

inline sys_time_t to_sys_time(const time &ti) {
	auto ela = ti.to_unix().elapsed();
	return {
		ela.seconds<decltype(sys_time_t::tv_sec)>(),
		ela.remaining_nanoseconds<decltype(sys_time_t::tv_usec)>() / 1000};
}

inline time now(int8_t zone = local_time_zone()) {
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return from_sys_time(tv, zone);
}

} // namespace _now

using namespace _now;

}} // namespace rua::posix

#endif
