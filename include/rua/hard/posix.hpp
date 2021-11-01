#ifndef _RUA_HARD_POSIX_HPP
#define _RUA_HARD_POSIX_HPP

#include <unistd.h>

namespace rua { namespace posix {

namespace _hard {

inline size_t num_cpus() {
	static auto n = ([]() -> size_t {
		auto n = sysconf(_SC_NPROCESSORS_ONLN);
		if (n < 0) {
			return 0;
		}
		return n;
	})();
	return n;
}

} // namespace _hard

using namespace _hard;

}} // namespace rua::posix

#endif