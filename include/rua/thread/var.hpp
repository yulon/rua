#ifndef _rua_thread_var_hpp
#define _rua_thread_var_hpp

#include "../util/macros.hpp"

#ifdef _WIN32

#include "var/win32.hpp"

namespace rua {

using thread_word_var = win32::thread_word_var;

template <typename T>
using thread_var = win32::thread_var<T>;

} // namespace rua

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "var/posix.hpp"

namespace rua {

using thread_word_var = posix::thread_word_var;

template <typename T>
using thread_var = posix::thread_var<T>;

} // namespace rua

#else

#include "var/uni.hpp"

namespace rua {

using thread_word_var = uni::thread_word_var;

template <typename T>
using thread_var = uni::thread_var<T>;

} // namespace rua

#endif

#endif
