#ifndef _RUA_CP_TLS_HPP
#define _RUA_CP_TLS_HPP

#include "../macros.hpp"

#ifdef _WIN32
	#include "tls/win32.hpp"
	namespace rua {
		namespace cp {
			using tls = win32::tls;
			using fls = win32::fls;
		}
	}
#elif defined(RUA_UNIX) || RUA_HAS_INC(<pthread.h>) || defined(_PTHREAD_H)
	#include "tls/posix.hpp"
	namespace rua {
		namespace cp {
			using tls = posix::tls;
		}
	}
#else
	#include "tls/uni.hpp"
	namespace rua {
		namespace cp {
			using tls = uni::tls;
		}
	}
#endif

#endif
