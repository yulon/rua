#ifndef _RUA_THREAD_THREAD_HPP
#define _RUA_THREAD_THREAD_HPP

#include "thread_id.hpp"

#include "../macros.hpp"

#ifdef _WIN32

#include "thread/win32.hpp"

namespace rua {
using thread = win32::thread;
}

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "thread/posix.hpp"

namespace rua {
using thread = posix::thread;
}

#endif

#endif
