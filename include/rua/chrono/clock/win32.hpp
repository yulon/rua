#ifndef _RUA_CHRONO_CLOCK_WIN32_HPP
#define _RUA_CHRONO_CLOCK_WIN32_HPP

#include "../time.hpp"

#include "../../dylib/win32.hpp"

#include <windows.h>

#include <cstdint>
#include <cstdlib>

namespace rua { namespace win32 {

namespace _clock {

inline time tick() {
	static LARGE_INTEGER start, freq;
	static bool ok = ([]() -> bool {
		return QueryPerformanceCounter(&start) &&
			   QueryPerformanceFrequency(&freq);
	})();
	if (ok) {
		LARGE_INTEGER end;
		if (QueryPerformanceCounter(&end)) {
			int64_t els = (end.QuadPart - start.QuadPart);
			int64_t els_s = els / freq.QuadPart;
			return time(duration_base(
				els_s,
				static_cast<int32_t>(
					els * 1000000000 / freq.QuadPart - els_s * 1000000000)));
		}
	}
	static dylib kernel32("kernel32.dll", false);
	static decltype(&GetTickCount64) GetTickCount64_ptr =
		kernel32["GetTickCount64"];
	if (GetTickCount64_ptr) {
		return ms(GetTickCount64_ptr());
	}
	return ms(GetTickCount());
}

static const date_t sys_epoch{1601, 1, 1, 0, 0, 0, 0, 0};

inline time now() {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return time(
		duration<100>(
			static_cast<int64_t>(ft.dwHighDateTime) << 32 | ft.dwLowDateTime),
		sys_epoch);
}

} // namespace _clock

using namespace _clock;

}} // namespace rua::win32

#endif
