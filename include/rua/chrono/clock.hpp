#ifndef _RUA_CHRONO_CLOCK_HPP
#define _RUA_CHRONO_CLOCK_HPP

#include "../macros.hpp"

#ifdef _WIN32

#include "clock/win32.hpp"

namespace rua {

RUA_FORCE_INLINE time tick() {
	return win32::tick();
}

RUA_FORCE_INLINE time now() {
	return win32::now();
}

} // namespace rua

#elif defined(RUA_DARWIN)

#include "clock/darwin.hpp"

namespace rua {

RUA_FORCE_INLINE time tick() {
	return darwin::tick();
}

RUA_FORCE_INLINE time now() {
	return darwin::now();
}

} // namespace rua

#elif defined(RUA_UNIX)

#include "clock/posix.hpp"

namespace rua {

RUA_FORCE_INLINE time tick() {
	return posix::tick();
}

RUA_FORCE_INLINE time now() {
	return posix::now();
}

} // namespace rua

#else

#error rua::clock: not supported this platform!

#endif

#endif
