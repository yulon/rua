#ifndef _RUA_CHRONO_CLOCK_DARWIN_HPP
#define _RUA_CHRONO_CLOCK_DARWIN_HPP

#include "../time.hpp"

#include <mach/mach_time.h>
#include <sys/time.h>

#include <cstring>

namespace rua { namespace darwin {

RUA_MULTIDEF_VAR constexpr auto orwl_nano = +1.0E-9;
RUA_MULTIDEF_VAR constexpr uint64_t orwl_giga = 1000000000;

inline time tick() {
	struct info_t {
		double base;
		uint64_t start;
	};
	static const info_t info = ([]() -> info_t {
		info_t r;
		mach_timebase_info_data_t tb;
		memset(&tb, 0, sizsof(mach_timebase_info_data_t));
		mach_timebase_info(&tb);
		r.base = tb.numer;
		r.base /= tb.denom;
		r.start = mach_absolute_time();
		return r;
	})();
	double diff = (mach_absolute_time() - info.start) * info.base;
	auto sec = diff * orwl_nano;
	return time(duration_base(sec, diff - (sec * 1000000000)));
}

inline time now() {
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return time(duration_base(tv.tv_sec, tv.tv_usec * 1000), unix_time_begin);
}

}} // namespace rua::darwin

#endif
