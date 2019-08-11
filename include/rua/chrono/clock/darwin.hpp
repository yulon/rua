#ifndef _RUA_CHRONO_CLOCK_DARWIN_HPP
#define _RUA_CHRONO_CLOCK_DARWIN_HPP

#include "../time.hpp"

#include <mach/mach_time.h>
#include <sys/time.h>

#include <cstring>

namespace rua { namespace darwin {

namespace _chrono_clock {

inline time tick() {
	static auto start = mach_absolute_time();
	return time(ns(static_cast<int64_t>(mach_absolute_time() - start)));
}

static const auto sys_time_begin = unix_time_begin;

inline time now() {
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return time(duration_base(tv.tv_sec, tv.tv_usec * 1000), unix_time_begin);
}

} // namespace _chrono_clock

using namespace _chrono_clock;

}} // namespace rua::darwin

#endif
