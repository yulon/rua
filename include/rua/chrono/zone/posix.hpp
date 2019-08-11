#ifndef _RUA_CHRONO_ZONE_POSIX_HPP
#define _RUA_CHRONO_ZONE_POSIX_HPP

#include "../time.hpp"

#include <time.h>

namespace rua { namespace posix {

namespace _chrono_zone {

inline int8_t local_time_zone() {
	return 0;
}

} // namespace _chrono_zone

using namespace _chrono_zone;

}} // namespace rua::posix

#endif
