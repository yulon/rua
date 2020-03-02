#ifndef _RUA_STDIO_WIN32_HPP
#define _RUA_STDIO_WIN32_HPP

#include "../io/sys_stream.hpp"
#include "../macros.hpp"
#include "../string/encoding/io/win32.hpp"

#include "windows.h"

#include <atomic>
#include <cassert>
#include <utility>

namespace rua { namespace win32 {

template <DWORD Ix>
inline void _set_stdio(sys_stream s) {
	auto h = s.native_handle();
	s.detach();

	SetStdHandle(Ix, h);

	static std::atomic<HANDLE> h_cache;
	auto old_h = h_cache.exchange(h);
	if (!old_h) {
		return;
	}
	CloseHandle(old_h);
}

namespace _stdio {

RUA_FORCE_INLINE sys_stream out() {
	return sys_stream(GetStdHandle(STD_OUTPUT_HANDLE), false);
}

RUA_FORCE_INLINE sys_stream err() {
	return sys_stream(GetStdHandle(STD_ERROR_HANDLE), false);
}

RUA_FORCE_INLINE sys_stream in() {
	return sys_stream(GetStdHandle(STD_INPUT_HANDLE), false);
}

RUA_FORCE_INLINE void set_out(sys_stream w) {
	_set_stdio<STD_OUTPUT_HANDLE>(std::move(w));
}

RUA_FORCE_INLINE void set_err(sys_stream w) {
	_set_stdio<STD_ERROR_HANDLE>(std::move(w));
}

RUA_FORCE_INLINE void set_in(sys_stream r) {
	_set_stdio<STD_INPUT_HANDLE>(std::move(r));
}

RUA_FORCE_INLINE u8_to_loc_writer sout() {
	return u8_to_loc_writer(out());
}

RUA_FORCE_INLINE u8_to_loc_writer serr() {
	return u8_to_loc_writer(err());
}

RUA_FORCE_INLINE loc_to_u8_reader sin() {
	return loc_to_u8_reader(in());
}

} // namespace _stdio

using namespace _stdio;

}} // namespace rua::win32

#endif
