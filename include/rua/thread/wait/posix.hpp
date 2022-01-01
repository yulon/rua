#ifndef _RUA_THREAD_WAIT_POSIX_HPP
#define _RUA_THREAD_WAIT_POSIX_HPP

#include "../basic/posix.hpp"

#include "../../sched/await.hpp"

#include <pthread.h>

namespace rua { namespace posix {

inline any_word thread::wait() {
	if (!_id) {
		return nullptr;
	}
	void *retval;
	if (await(pthread_join, _id, &retval)) {
		return nullptr;
	}
	reset();
	return retval;
}

}} // namespace rua::posix

#endif
