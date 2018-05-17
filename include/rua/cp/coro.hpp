#ifndef _RUA_CP_CORO_HPP
#define _RUA_CP_CORO_HPP

#include "../macros.hpp"

#ifdef _WIN32
	#include "coro/win32.hpp"
	namespace rua {
		namespace cp {
			using coro = win32::coro;
		}
	}
#elif defined(__unix__) || RUA_HAS_INC(<ucontext.h>) || defined(_UCONTEXT_H)
	#include "coro/uc.hpp"
	namespace rua {
		namespace cp {
			using coro = uc::coro;
		}
	}
#else
	#include "coro/uni.hpp"
	namespace rua {
		namespace cp {
			using coro = uni::coro;
		}
	}
#endif

#endif
