#ifndef _RUA_CHRONO_CLOCK_POSIX_HPP
#define _RUA_CHRONO_CLOCK_POSIX_HPP

#include "../time.hpp"

#include <sys/time.h>

#include <time.h>

namespace rua { namespace posix {

namespace _chrono_clock {

inline time tick() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return time(duration_base(ts.tv_sec, ts.tv_nsec));
}

static const auto sys_start_date = unix_start_date;

inline time now() {
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return time(duration_base(tv.tv_sec, tv.tv_usec * 1000), unix_start_date);
}

} // namespace _chrono_clock

using namespace _chrono_clock;

}} // namespace rua::posix

#endif
