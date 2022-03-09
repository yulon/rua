#ifndef _RUA_THREAD_SLEEP_WIN32_HPP
#define _RUA_THREAD_SLEEP_WIN32_HPP

#include "../../time/duration.hpp"
#include "../../util.hpp"

#include <windows.h>

#include <cassert>

namespace rua { namespace win32 {

namespace _sleep {

inline void sleep(duration timeout) {
	assert(timeout >= 0);

	Sleep(timeout.milliseconds<DWORD, INFINITE>());
}

} // namespace _sleep

using namespace _sleep;

}} // namespace rua::win32

#endif
