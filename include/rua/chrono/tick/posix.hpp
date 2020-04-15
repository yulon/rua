#ifndef _RUA_CHRONO_TICK_POSIX_HPP
#define _RUA_CHRONO_TICK_POSIX_HPP

#include "../time.hpp"

#include "../../macros.hpp"

#include <time.h>

namespace rua { namespace posix {

namespace _tick {

RUA_FORCE_INLINE time tick() {
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return time(ts);
}

} // namespace _tick

using namespace _tick;

}} // namespace rua::posix

#endif
