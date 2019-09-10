#ifndef _RUA_THREAD_THIS_THREAD_HPP
#define _RUA_THREAD_THIS_THREAD_HPP

#include "thread.hpp"
#include "thread_id.hpp"

#include "../macros.hpp"

#ifdef _WIN32

#include "this_thread/win32.hpp"

namespace rua {
using namespace win32::_this_thread;
}

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "this_thread/posix.hpp"

namespace rua {
using namespace posix::_this_thread;
}

#endif

#endif
