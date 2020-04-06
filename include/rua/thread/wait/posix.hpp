#ifndef _RUA_THREAD_WAIT_POSIX_HPP
#define _RUA_THREAD_WAIT_POSIX_HPP

#include "../basic/posix.hpp"

#include "../../sched/wait/uni.hpp"

#include <pthread.h>

namespace rua { namespace posix {

inline any_word thread::wait_for_exit() {
	if (!_id) {
		return nullptr;
	}
	void *retval;
	if (wait(pthread_join, _id, &retval)) {
		return nullptr;
	}
	reset();
	return retval;
}

}} // namespace rua::posix

#endif
