#ifndef _rua_time_tick_posix_hpp
#define _rua_time_tick_posix_hpp

#include "../real.hpp"

#include "../../util.hpp"

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
