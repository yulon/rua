#ifndef _RUA_THREAD_THREAD_ID_POSIX_HPP
#define _RUA_THREAD_THREAD_ID_POSIX_HPP

#include <pthread.h>

namespace rua { namespace posix {

using tid_t = pthread_t;

}} // namespace rua::posix

#endif
