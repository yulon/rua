#ifndef _rua_sys_info_posix_hpp
#define _rua_sys_info_posix_hpp

#include "../../string/conv.hpp"
#include "../../string/view.hpp"
#include "../../vernum.hpp"

#include <sys/utsname.h>

namespace rua { namespace posix {

namespace _sys_version {

inline vernum sys_version() {
	static auto const cache = []() -> vernum {
		utsname inf;
		if (uname(&inf) < 0) {
			return 0;
		}
		string_view ver_str(inf.release);
		uint16_t nums[4];
		size_t num_ix = 0;
		size_t start = 0;
		for (size_t i = 0; i < ver_str.length(); ++i) {
			size_t sz;
			if (ver_str[i] == '.') {
				sz = i - start;
			} else if (i == ver_str.length() - 1) {
				sz = ver_str.length() - start;
			} else {
				continue;
			}
			if (!sz) {
				continue;
			}
			nums[num_ix++] = stoi(to_string(ver_str.substr(start, sz)));
			if (num_ix == 4) {
				break;
			}
			start = i + 1;
		}
		return {nums[0], nums[1], nums[2], nums[3]};
	}();
	return cache;
}

} // namespace _sys_version

using namespace _sys_version;

}} // namespace rua::posix

#endif
