#ifndef _RUA_CHRONO_TICK_WIN32_HPP
#define _RUA_CHRONO_TICK_WIN32_HPP

#include "../time.hpp"

#include "../../dylib/win32.hpp"
#include "../../types/util.hpp"

#include <windows.h>

namespace rua { namespace win32 {

namespace _tick {

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
			return time(s::from_s_and_extra_ns_counts(
				els_s,
				static_cast<int32_t>(
					els * 1000000000 / freq.QuadPart - els_s * 1000000000)));
		}
	}
	static dylib kernel32("kernel32.dll", false);
	static decltype(&GetTickCount64) GetTickCount64_ptr =
		kernel32["GetTickCount64"];
	if (GetTickCount64_ptr) {
		return time(ms(GetTickCount64_ptr()));
	}
	return time(ms(GetTickCount()));
}

} // namespace _tick

using namespace _tick;

}} // namespace rua::win32

#endif
