#ifndef _RUA_THREAD_WAIT_POSIX_HPP
#define _RUA_THREAD_WAIT_POSIX_HPP

#include "../core/posix.hpp"
#include "../parallel.hpp"

#include <pthread.h>

namespace rua { namespace posix {

inline awaitable<generic_word> thread::RUA_OPERATOR_AWAIT() const {
	if (!_id) {
		return 0;
	}
	auto id = _id;
	return parallel([id]() -> generic_word {
		void *retval;
		pthread_join(id, &retval);
		return retval;
	});
}

}} // namespace rua::posix

#endif
