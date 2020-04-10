#ifndef _RUA_SCHED_WAIT_WIN32_HPP
#define _RUA_SCHED_WAIT_WIN32_HPP

#include "../reg_wait/win32.hpp"

#include "../../sched/scheduler.hpp"
#include "../../sync/chan.hpp"

namespace rua { namespace win32 {

namespace _wait {

inline bool wait(HANDLE handle, duration timeout = duration_max()) {
	assert(handle);

	auto sch = this_scheduler();
	if (sch.type_is<rua::thread_scheduler>()) {
		return WaitForSingleObject(
				   handle, timeout.milliseconds<DWORD, INFINITE>()) !=
			   WAIT_TIMEOUT;
	}
	auto ch = std::make_shared<chan<bool>>();
	reg_wait(
		handle, [=](bool r) mutable { *ch << r; }, timeout);
	return ch->pop(sch);
}

} // namespace _wait

using namespace _wait;

}} // namespace rua::win32

#endif
