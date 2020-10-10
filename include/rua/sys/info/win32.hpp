#ifndef _RUA_SYS_INFO_WIN32_HPP
#define _RUA_SYS_INFO_WIN32_HPP

#include "../../dylib/win32.hpp"
#include "../../vernum.hpp"

#include <windows.h>

#include <vector>

namespace rua { namespace win32 {

namespace _sys_version {

inline vernum sys_version() {
	static auto const cache = []() -> vernum {
		static dylib version_dll("version.dll");

		auto ntoskrnl = L"ntoskrnl.exe";

		static auto GetFileVersionInfoSizeW_ptr =
			version_dll.RUA_DYFN(GetFileVersionInfoSizeW);
		if (!GetFileVersionInfoSizeW_ptr) {
			return 0;
		}

		DWORD sz, h;
		sz = GetFileVersionInfoSizeW_ptr(ntoskrnl, &h);
		if (sz == 0) {
			return 0;
		}

		static auto GetFileVersionInfoW_ptr =
			version_dll.RUA_DYFN(GetFileVersionInfoW);
		if (!GetFileVersionInfoW_ptr) {
			return 0;
		}

		std::vector<uint8_t> raw_ver_info(sz + 1);
		if (!GetFileVersionInfoW_ptr(
				ntoskrnl,
				0,
				sz,
				reinterpret_cast<LPVOID>(raw_ver_info.data()))) {
			return 0;
		}

		static auto VerQueryValueW_ptr = version_dll.RUA_DYFN(VerQueryValueW);
		if (!VerQueryValueW_ptr) {
			return 0;
		}

		VS_FIXEDFILEINFO *ver_info;
		UINT len;
		if (!VerQueryValueW_ptr(
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
