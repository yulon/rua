#ifndef _RUA_THREAD_CREATIONAL_HPP
#define _RUA_THREAD_CREATIONAL_HPP

#include "../macros.hpp"

#ifdef _WIN32

#include "creational/win32.hpp"

namespace rua {

using thread = win32::thread;

using namespace win32::_this_thread_id;

} // namespace rua

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "creational/posix.hpp"

namespace rua {

using thread = posix::thread;

using namespace posix::_this_thread;

} // namespace rua

#endif

#endif
