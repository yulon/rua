#ifndef _RUA_CHRONO_ZONE_POSIX_HPP
#define _RUA_CHRONO_ZONE_POSIX_HPP

#include "../time.hpp"

#include <time.h>

namespace rua { namespace posix {

namespace _zone {

inline int8_t local_time_zone() {
	return 0;
}

} // namespace _zone

using namespace _zone;

}} // namespace rua::posix

#endif
