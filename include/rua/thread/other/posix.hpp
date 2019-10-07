#ifndef _RUA_THREAD_OTHER_POSIX_HPP
#define _RUA_THREAD_OTHER_POSIX_HPP

#include "../creational/posix.hpp"

#include "../../sched/block_call.hpp"

#include <pthread.h>

namespace rua { namespace posix {

inline any_word thread::wait_for_exit() {
	if (!_id) {
		return nullptr;
	}
	void *retval;
	if (block_call(pthread_join, _id, &retval)) {
		return nullptr;
	}
	reset();
	return retval;
}

}} // namespace rua::posix

#endif
