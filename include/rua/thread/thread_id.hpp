#ifndef _RUA_THREAD_THREAD_ID_HPP
#define _RUA_THREAD_THREAD_ID_HPP

#include "../macros.hpp"

#ifdef _WIN32

#include "thread_id/win32.hpp"

namespace rua {
using tid_t = win32::tid_t;
}

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "thread_id/posix.hpp"

namespace rua {
using tid_t = posix::tid_t;
}

#endif

#endif
