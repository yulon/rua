#ifndef _RUA_THREAD_DOZER_HPP
#define _RUA_THREAD_DOZER_HPP

#include "../util/macros.hpp"

#ifdef _WIN32

#include "dozer/win32.hpp"

namespace rua {

using thread_dozer = win32::thread_dozer;

} // namespace rua

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "dozer/posix.hpp"

namespace rua {

using thread_dozer = posix::thread_dozer;

} // namespace rua

#endif

#endif
