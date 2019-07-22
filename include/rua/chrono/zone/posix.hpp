#ifndef _RUA_CHRONO_TIMES_POSIX_HPP
#define _RUA_CHRONO_TIMES_POSIX_HPP

#include "../time.hpp"

#include <time.h>

namespace rua { namespace posix {

inline int8_t local_time_zone() {
	return 0;
}

}} // namespace rua::posix

#endif
