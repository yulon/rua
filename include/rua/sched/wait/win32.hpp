#ifndef _RUA_SCHED_WAIT_WIN32_HPP
#define _RUA_SCHED_WAIT_WIN32_HPP

#include "../reg_wait/win32.hpp"

#include "../../sched/suspender.hpp"
#include "../../sync/chan.hpp"

namespace rua { namespace win32 {

namespace _wait {

inline bool
wait(suspender_i spdr, HANDLE handle, duration timeout = duration_max()) {
	assert(spdr);
	assert(handle);

	auto ch = std::make_shared<chan<bool>>();
	reg_wait(
		handle, [=](bool r) mutable { *ch << r; }, timeout);
	return ch->pop(std::move(spdr));
}

inline bool wait(HANDLE handle, duration timeout = duration_max()) {
	assert(handle);

	auto spdr = this_suspender();
	if (spdr.type_is<rua::thread_suspender>()) {
		return WaitForSingleObject(
				   handle, timeout.milliseconds<DWORD, INFINITE>()) !=
			   WAIT_TIMEOUT;
	}
	return wait(std::move(spdr), handle, timeout);
}

} // namespace _wait

using namespace _wait;

}} // namespace rua::win32

#endif
