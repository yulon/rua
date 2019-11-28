#ifndef _RUA_THREAD_THREAD_OTHER_HPP
#define _RUA_THREAD_THREAD_OTHER_HPP

#include "../../macros.hpp"

#ifdef _WIN32

#include "other/win32.hpp"

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "other/posix.hpp"

#endif

#endif
