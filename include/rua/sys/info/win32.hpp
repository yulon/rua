#ifndef _rua_sys_info_win32_hpp
#define _rua_sys_info_win32_hpp

#include "../../dylib/win32.hpp"
#include "../../vernum.hpp"

#include <windows.h>

#include <vector>

namespace rua { namespace win32 {

namespace _sys_version {

inline vernum sys_version() {
	static auto const cache = []() -> vernum {
		static dylib version_dll("version.dll");

		static auto get_file_version_info_size_w =
			version_dll.RUA_DYFN(GetFileVersionInfoSizeW);
		if (!get_file_version_info_size_w) {
			return 0;
		}

		auto version_dll_w = L"version.dll";

		DWORD sz, h;
		sz = get_file_version_info_size_w(version_dll_w, &h);
		if (sz == 0) {
			return 0;
		}

		static auto get_file_version_info_w =
			version_dll.RUA_DYFN(GetFileVersionInfoW);
		if (!get_file_version_info_w) {
			return 0;
		}

		std::vector<uint8_t> raw_ver_info(sz + 1);
		if (!get_file_version_info_w(
				version_dll_w,
				0,
				sz,
				reinterpret_cast<LPVOID>(raw_ver_info.data()))) {
			return 0;
		}

		static auto ver_query_value_w = version_dll.RUA_DYFN(VerQueryValueW);
		if (!ver_query_value_w) {
			return 0;
		}

		VS_FIXEDFILEINFO *ver_info;
		UINT len;
		if (!ver_query_value_w(
				reinterpret_cast<LPCVOID>(raw_ver_info.data()),
				L"\\",
				reinterpret_cast<LPVOID *>(&ver_info),
				&len)) {
			return 0;
		}

		return {
			HIWORD(ver_info->dwFileVersionMS),
			LOWORD(ver_info->dwFileVersionMS),
			HIWORD(ver_info->dwFileVersionLS),
			LOWORD(ver_info->dwFileVersionLS)};
	}();
	return cache;
}

} // namespace _sys_version

using namespace _sys_version;

}} // namespace rua::win32

#endif
