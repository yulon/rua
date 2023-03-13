#ifndef _rua_time_tick_darwin_hpp
#define _rua_time_tick_darwin_hpp

#include "../real.hpp"

#include "../../util.hpp"

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
