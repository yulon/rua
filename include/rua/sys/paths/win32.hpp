#ifndef _rua_sys_paths_win32_hpp
#define _rua_sys_paths_win32_hpp

#include "../../file/win32.hpp"
#include "../../process/win32.hpp"
#include "../../string/split.hpp"

#include <windows.h>

namespace rua { namespace win32 {

namespace _sys_paths {

inline file_path user_dir() {
	file_path r(get_env("USERPROFILE"));
	if (r) {
		return r;
	}
	r = get_env("HOME");
	return r;
}

inline file_path configs_dir() {
	file_path r(get_env("APPDATA"));
	if (r) {
		return r;
	}
	r = user_dir();
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
	auto pth_w = _get_dir_w(GetTempPathW);
	if (pth_w.find(L'~') != std::wstring::npos) {
		pth_w = _get_dir_w(GetLongPathNameW, std::move(pth_w));
	}
	file_path r(w2u(pth_w));
	if (r) {
		return r;
	}
	r = get_env("TEMP");
	if (r) {
		return r;
	}
	r = get_env("TMP");
	if (r) {
		return r;
	}
	r = get_env("APPDATA");
	if (r) {
		r = {r, "Local", "Temp"};
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

}} // namespace rua::win32

#endif
