#ifndef _RUA_CHRONO_NOW_POSIX_HPP
#define _RUA_CHRONO_NOW_POSIX_HPP

#include "../time.hpp"

#include "../../macros.hpp"
#include "../../types/util.hpp"

#include <sys/time.h>
#include <time.h>

namespace rua { namespace posix {

namespace _now {

static const auto sys_epoch = unix_epoch;

RUA_FORCE_INLINE int8_t local_time_zone() {
	tzset();
	return static_cast<int8_t>(timezone / 3600 + 16);
}

RUA_FORCE_INLINE time now(int8_t zone = local_time_zone()) {
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return time(
		s::from_s_and_extra_ns_counts(tv.tv_sec, tv.tv_usec * 1000),
		zone,
		unix_epoch);
}

} // namespace _now

using namespace _now;

}} // namespace rua::posix

#endif
