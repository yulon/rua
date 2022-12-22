#ifndef _RUA_THREAD_ID_HPP
#define _RUA_THREAD_ID_HPP

#include "../util/macros.hpp"

#ifdef _WIN32

#include "id/win32.hpp"

namespace rua {

using tid_t = win32::tid_t;
using namespace win32::_this_tid;

} // namespace rua

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "id/posix.hpp"

namespace rua {

using tid_t = posix::tid_t;
using namespace posix::_this_tid;

} // namespace rua

#endif

#endif
