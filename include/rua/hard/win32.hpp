#ifndef _RUA_HARD_WIN32_HPP
#define _RUA_HARD_WIN32_HPP

#include <windows.h>

namespace rua { namespace win32 {

namespace _hard {

inline size_t num_cpus() {
	static auto n = ([]() -> size_t {
		SYSTEM_INFO si;
		memset(&si, 0, sizeof(si));
		GetSystemInfo(&si);
		return si.dwNumberOfProcessors;
	})();
	return n;
}

} // namespace _hard

using namespace _hard;

}} // namespace rua::win32

#endif