#ifndef _RUA_CHRONO_TICK_POSIX_HPP
#define _RUA_CHRONO_TICK_POSIX_HPP

#include "../time.hpp"

#include <time.h>

namespace rua { namespace posix {

namespace _tick {

inline time tick() {
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return time(s::from_s_and_extra_ns_counts(ts.tv_sec, ts.tv_nsec));
}

} // namespace _tick

using namespace _tick;

}} // namespace rua::posix

#endif
