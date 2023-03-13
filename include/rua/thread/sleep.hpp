#ifndef _rua_thread_sleep_hpp
#define _rua_thread_sleep_hpp

#include "../util/macros.hpp"

#ifdef _WIN32

#include "sleep/win32.hpp"

namespace rua {

using namespace win32::_sleep;

} // namespace rua

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "sleep/posix.hpp"

namespace rua {

using namespace posix::_sleep;

} // namespace rua

#endif

#endif
