#ifndef _RUA_CHRONO_CLOCK_POSIX_HPP
#define _RUA_CHRONO_CLOCK_POSIX_HPP

#include "../time.hpp"

#include <sys/time.h>

#include <time.h>

namespace rua { namespace posix {

namespace _clock {

inline time tick() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return time(duration_base(ts.tv_sec, ts.tv_nsec));
}

static const auto sys_epoch = unix_epoch;

inline time now() {
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return time(duration_base(tv.tv_sec, tv.tv_usec * 1000), unix_epoch);
}

} // namespace _clock

using namespace _clock;

}} // namespace rua::posix

#endif
