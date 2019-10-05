#ifndef _RUA_THREAD_ID_POSIX_HPP
#define _RUA_THREAD_ID_POSIX_HPP

#include "../../macros.hpp"

#include <pthread.h>

namespace rua { namespace posix {

using tid_t = pthread_t;

namespace _this_thread_id {

RUA_FORCE_INLINE tid_t this_thread_id() {
	return pthread_self();
}

} // namespace _this_thread_id

using namespace _this_thread_id;

}} // namespace rua::posix

#endif
