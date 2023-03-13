#ifndef _rua_thread_id_posix_hpp
#define _rua_thread_id_posix_hpp

#include <pthread.h>

namespace rua { namespace posix {

using tid_t = pthread_t;

namespace _this_tid {

inline tid_t this_tid() {
	return pthread_self();
}

} // namespace _this_tid

using namespace _this_tid;

}} // namespace rua::posix

#endif
