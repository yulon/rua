#ifndef _RUA_SYS_HANDLE_WAIT_FOR_FINISH_WIN32_HPP
#define _RUA_SYS_HANDLE_WAIT_FOR_FINISH_WIN32_HPP

#include "../on_finish/win32.hpp"

#include "../../../sched/suspender.hpp"
#include "../../../sync/chan.hpp"

#include <memory>

namespace rua { namespace win32 {

namespace _wait_for_sys_handle_finish {

inline bool wait_for_sys_handle_finish(
	suspender_i spdr, HANDLE handle, duration timeout = duration_max()) {
	assert(spdr);
	assert(handle);

	if (spdr.type_is<rua::thread_suspender>()) {
		return WaitForSingleObject(
				   handle, timeout.milliseconds<DWORD, INFINITE>()) !=
			   WAIT_TIMEOUT;
	}
	auto ch_ptr = new chan<bool>;
	std::unique_ptr<chan<bool>> ch_uptr(ch_ptr);
	on_sys_handle_finish(
		handle, [=](bool r) mutable { *ch_ptr << r; }, timeout);
	return ch_ptr->pop(std::move(spdr));
}

inline bool
wait_for_sys_handle_finish(HANDLE handle, duration timeout = duration_max()) {
	assert(handle);

	return wait_for_sys_handle_finish(this_suspender(), handle, timeout);
}

} // namespace _wait_for_sys_handle_finish

using namespace _wait_for_sys_handle_finish;

}} // namespace rua::win32

#endif
