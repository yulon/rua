#ifndef _RUA_THREAD_LOC_HPP
#define _RUA_THREAD_LOC_HPP

#include "../macros.hpp"

#ifdef _WIN32

#include "loc/win32.hpp"

namespace rua {

using thread_loc_word = win32::thread_loc_word;

template <typename T>
using thread_loc = win32::thread_loc<T>;

} // namespace rua

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "loc/posix.hpp"

namespace rua {

using thread_loc_word = posix::thread_loc_word;

template <typename T>
using thread_loc = posix::thread_loc<T>;

} // namespace rua

#else

#include "loc/uni.hpp"

namespace rua {

using thread_loc_word = uni::thread_loc_word;

template <typename T>
using thread_loc = uni::thread_loc<T>;

} // namespace rua

#endif

#endif