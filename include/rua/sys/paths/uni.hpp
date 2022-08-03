#ifndef _RUA_SYS_PATHS_UNI_HPP
#define _RUA_SYS_PATHS_UNI_HPP

#include "../../file.hpp"
#include "../../process.hpp"
#include "../../string/split.hpp"

namespace rua { namespace uni {

namespace _sys_paths {

inline file_path user_dir() {
	return get_env("HOME");
}

inline file_path configs_dir() {
	auto r = user_dir();
	if (r) {
		r /= ".config";
		return r;
	}
	return r;
}

inline file_path config_dir(string_view app_name = "") {
	auto r = configs_dir();
	if (r) {
		r /= app_name.length() ? app_name
							   : split(this_process().path().back(), '.')[0];
		return r;
	}
	r = this_process().path().rm_back() / ".config";
	return r;
}

inline file_path temp_dir() {
	file_path r(get_env("TEMP"));
	if (r) {
		return r;
	}
	r = get_env("TMP");
	if (r) {
		return r;
	}
	r = user_dir();
	if (r) {
		r /= ".tmp";
		return r;
	}
	r = this_process().path().rm_back() / ".tmp";
	return r;
}

} // namespace _sys_paths

using namespace _sys_paths;

}} // namespace rua::uni

#endif
