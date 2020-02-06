#ifndef _RUA_THREAD_VAR_HPP
#define _RUA_THREAD_VAR_HPP

#include "../macros.hpp"

#ifdef _WIN32

#include "var/win32.hpp"

namespace rua {

using thread_storage = win32::thread_storage;

template <typename T>
using thread_var = win32::thread_var<T>;

} // namespace rua

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "var/posix.hpp"

namespace rua {

using thread_storage = posix::thread_storage;

template <typename T>
using thread_var = posix::thread_var<T>;

} // namespace rua

#else

#include "var/uni.hpp"

namespace rua {

using thread_storage = uni::thread_storage;

template <typename T>
using thread_var = uni::thread_var<T>;

} // namespace rua

#endif

#endif
