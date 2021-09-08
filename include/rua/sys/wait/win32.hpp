#ifndef _RUA_SYS_WAIT_WIN32_HPP
#define _RUA_SYS_WAIT_WIN32_HPP

#include "../listen/win32.hpp"

#include "../../sched/suspender.hpp"
#include "../../sync/chan.hpp"

#include <memory>

namespace rua { namespace win32 {

namespace _sys_wait {

inline bool
sys_wait(suspender_i spdr, HANDLE handle, duration timeout = duration_max()) {
	assert(spdr);
	assert(handle);

	if (spdr.type_is<rua::thread_suspender>()) {
		return WaitForSingleObject(
				   handle, timeout.milliseconds<DWORD, INFINITE>()) !=
			   WAIT_TIMEOUT;
	}
	auto ch_ptr = new chan<bool>;
	std::unique_ptr<chan<bool>> ch_uptr(ch_ptr);
	sys_listen(
		handle, [=](bool r) mutable { *ch_ptr << r; }, timeout);
	return ch_ptr->pop(std::move(spdr));
}

inline bool sys_wait(HANDLE handle, duration timeout = duration_max()) {
	assert(handle);

	return sys_wait(this_suspender(), handle, timeout);
}

} // namespace _sys_wait

using namespace _sys_wait;

}} // namespace rua::win32

#endif
