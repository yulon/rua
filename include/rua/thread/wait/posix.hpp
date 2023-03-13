#ifndef _rua_thread_wait_posix_hpp
#define _rua_thread_wait_posix_hpp

#include "../core/posix.hpp"
#include "../parallel.hpp"

#include <pthread.h>

namespace rua { namespace posix {

inline future<generic_word> thread::RUA_OPERATOR_AWAIT() const {
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
