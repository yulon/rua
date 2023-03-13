#ifndef _rua_thread_wait_hpp
#define _rua_thread_wait_hpp

#include "../util/macros.hpp"

#if !defined(_WIN32) &&                                                        \
	(defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H))

#include "wait/posix.hpp"

#endif

#endif
