#ifndef _RUA_THREAD_SLEEP_POSIX_HPP
#define _RUA_THREAD_SLEEP_POSIX_HPP

#include "../../time/duration.hpp"
#include "../../util.hpp"

#include <time.h>

#include <cassert>

namespace rua { namespace posix {

namespace _sleep {

inline void sleep(duration timeout) {
	assert(timeout >= 0);

	auto dur = timeout.c_timespec();
	timespec rem;
	while (nanosleep(&dur, &rem) == -1) {
		dur = rem;
	}
}

} // namespace _sleep

using namespace _sleep;

}} // namespace rua::posix

#endif
