#ifndef _RUA_CP_TLS_HPP
#define _RUA_CP_TLS_HPP

#include "tls/uni.hpp"

#ifdef __has_include
	#if __has_include(<pthread.h>)
		#include "tls/posix.hpp"
	#endif
#endif

#include "../macros.hpp"

#if defined(_WIN32)
	#include "tls/win32.hpp"
	namespace rua {
		namespace cp {
			using tls = win32::tls;
			using fls = win32::fls;
		}
	}
#elif defined(RUA_UNIX)
	#include "tls/posix.hpp"
	namespace rua {
		namespace cp {
			using tls = posix::tls;
		}
	}
#else
	namespace rua {
		namespace cp {
			using tls = uni::tls;
		}
	}
#endif

#endif
