#ifndef _RUA_CHRONO_NOW_POSIX_HPP
#define _RUA_CHRONO_NOW_POSIX_HPP

#include "../time.hpp"

#include <sys/time.h>

#include <cstdint>

namespace rua { namespace posix {

namespace _now {

static const auto sys_epoch = unix_epoch;

inline time now() {
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return time(duration(tv.tv_sec, tv.tv_usec * 1000), unix_epoch);
}

inline int8_t local_time_zone() {
	return 0;
}

inline time local_now() {
	return now();
}

} // namespace _now

using namespace _now;

}} // namespace rua::posix

#endif
