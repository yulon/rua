#ifndef _RUA_TIME_TICK_WIN32_HPP
#define _RUA_TIME_TICK_WIN32_HPP

#include "../real.hpp"

#include "../../dylib/win32.hpp"
#include "../../util.hpp"

#include <windows.h>

namespace rua { namespace win32 {

namespace _tick {

inline duration tick() {
	static auto kernel32 = dylib::from_loaded_or_load("kernel32.dll");
	static decltype(&GetTickCount64) GetTickCount64_ptr =
		kernel32["GetTickCount64"];
	if (GetTickCount64_ptr) {
		return GetTickCount64_ptr();
	}

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
			return {
				els_s,
				static_cast<int32_t>(
					els * 1000000000 / freq.QuadPart - els_s * 1000000000)};
		}
	}

	return GetTickCount();
}

} // namespace _tick

using namespace _tick;

}} // namespace rua::win32

#endif
