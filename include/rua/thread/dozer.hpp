#ifndef _RUA_THREAD_DOZER_HPP
#define _RUA_THREAD_DOZER_HPP

#include "../util/macros.hpp"

#ifdef _WIN32

#include "dozer/win32.hpp"

namespace rua {

using dozer = win32::dozer;
using waker = win32::waker;

} // namespace rua

#elif defined(RUA_DARWIN)

#include "dozer/darwin.hpp"

namespace rua {

using dozer = darwin::dozer;
using waker = darwin::waker;

} // namespace rua

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "dozer/posix.hpp"

namespace rua {

using dozer = posix::dozer;
using waker = posix::waker;

} // namespace rua

#endif

#endif
