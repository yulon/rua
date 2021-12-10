#ifndef _RUA_TIME_TICK_POSIX_HPP
#define _RUA_TIME_TICK_POSIX_HPP

#include "../real.hpp"

#include "../../macros.hpp"

#include <time.h>

namespace rua { namespace posix {

namespace _tick {

inline duration tick() {
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts;
}

} // namespace _tick

using namespace _tick;

}} // namespace rua::posix

#endif
