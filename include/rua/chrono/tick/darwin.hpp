#ifndef _RUA_CHRONO_TICK_DARWIN_HPP
#define _RUA_CHRONO_TICK_DARWIN_HPP

#include "../time.hpp"

#include "../../macros.hpp"

#include <mach/mach_time.h>

namespace rua { namespace darwin {

namespace _tick {

inline duration tick() {
	static auto start = mach_absolute_time();
	return nanoseconds(static_cast<int64_t>(mach_absolute_time() - start));
}

} // namespace _tick

using namespace _tick;

}} // namespace rua::darwin

#endif
