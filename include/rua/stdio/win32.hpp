#ifndef _rua_stdio_win32_hpp
#define _rua_stdio_win32_hpp

#include "../string/codec/stream/win32.hpp"
#include "../sys/stream/win32.hpp"
#include "../util.hpp"

#include "windows.h"

#include <atomic>
#include <cassert>

namespace rua { namespace win32 {

namespace _stdio {

template <DWORD Id>
class stdio_stream : public sys_stream {
public:
	stdio_stream() : sys_stream(GetStdHandle(Id)) {}

	stdio_stream &operator=(const sys_stream &s) {
		static_cast<sys_stream &>(*this) = s.dup();

		auto h = native_handle();
		SetStdHandle(Id, native_handle());

		static std::atomic<HANDLE> h_cache;
		auto old_h = h_cache.exchange(h);
		if (!old_h) {
			return *this;
		}
		CloseHandle(old_h);
		return *this;
	}

	future<> close() override {
		auto $ = self().template as<stdio_stream>();
		return stream::close() >> [=]() { $->operator=(INVALID_HANDLE_VALUE); };
	}
};

using stdout_stream = stdio_stream<STD_OUTPUT_HANDLE>;
using stderr_stream = stdio_stream<STD_ERROR_HANDLE>;
using stdin_stream = stdio_stream<STD_INPUT_HANDLE>;

inline stdout_stream &out() {
	static stdout_stream s;
	return s;
}

inline stderr_stream &err() {
	static stderr_stream s;
	return s;
}

inline stdin_stream &in() {
	static stdin_stream s;
	return s;
}

inline stdout_stream &sout() {
	return out();
}

inline stderr_stream &serr() {
	return err();
}

inline stdin_stream &sin() {
	return in();
}

/*inline u2l_writer &sout() {
	static u2l_writer s(out());
	return s;
}

inline u2l_writer &serr() {
	static u2l_writer s(err());
	return s;
}

inline l2u_reader &sin() {
	static l2u_reader s(in());
	return s;
}*/

} // namespace _stdio

using namespace _stdio;

}} // namespace rua::win32

#endif
