#ifndef _RUA_THREAD_THREAD_SCHEDULER_HPP
#define _RUA_THREAD_THREAD_SCHEDULER_HPP

#include "../macros.hpp"

#ifdef _WIN32

#include "thread_scheduler/win32.hpp"

namespace rua {
using thread_scheduler = win32::thread_scheduler;
}

#elif defined(RUA_DARWIN)

#include "thread_scheduler/darwin.hpp"

namespace rua {
using thread_scheduler = darwin::thread_scheduler;
}

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "thread_scheduler/posix.hpp"

namespace rua {
using thread_scheduler = posix::thread_scheduler;
}

#endif

#endif
