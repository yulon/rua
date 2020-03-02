#ifndef _RUA_THREAD_WAIT_HPP
#define _RUA_THREAD_WAIT_HPP

#include "../macros.hpp"

#ifdef _WIN32

#include "wait/win32.hpp"

#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)

#include "wait/posix.hpp"

#endif

#endif
