#ifndef _rua_hard_win32_hpp
#define _rua_hard_win32_hpp

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