#ifndef _RUA_THREAD_WAIT_HPP
#define _RUA_THREAD_WAIT_HPP

#include "../util/macros.hpp"

#if !defined(_WIN32) &&                                                        \
	(defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H))

#include "wait/posix.hpp"

#endif

#endif
