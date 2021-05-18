#ifndef _RUA_THREAD_SUSPENDER_HPP
#define _RUA_THREAD_SUSPENDER_HPP

#include "../macros.hpp"

#ifdef _WIN32

#include "suspender/win32.hpp"

namespace rua {

using thread_suspender = win32::thread_suspender;

} // namespace rua

#elif defined(RUA_DARWIN)

#include "suspender/darwin.hpp"

namespace rua {

using thread_suspender = darwin::thread_suspender;

} // namespace rua

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "suspender/posix.hpp"

namespace rua {

using thread_suspender = posix::thread_suspender;

} // namespace rua

#endif

#endif
