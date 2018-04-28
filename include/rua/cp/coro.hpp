#ifndef _RUA_CP_CORO_HPP
#define _RUA_CP_CORO_HPP

#include "coro/uni.hpp"

#if defined(_WIN32) && !defined(RUA_CP_CORO_USING_UNI_IMPL)
	#include "coro/win32.hpp"
	namespace rua {
		namespace cp {
			using coro = win32::coro;
		}
	}
#elif defined(__unix__)
	#include "coro/uc.hpp"
	namespace rua {
		namespace cp {
			using coro = uc::coro;
		}
	}
#else
	namespace rua {
		namespace cp {
			using coro = uni::coro;
		}
	}
#endif

#endif
