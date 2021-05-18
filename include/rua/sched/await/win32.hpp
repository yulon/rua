#ifndef _RUA_SCHED_AWAIT_WIN32_HPP
#define _RUA_SCHED_AWAIT_WIN32_HPP

#include "../async/win32.hpp"

#include "../../sched/suspender.hpp"
#include "../../sync/chan.hpp"

#include <memory>

namespace rua { namespace win32 {

namespace _wait {

inline bool
await(suspender_i spdr, HANDLE handle, duration timeout = duration_max()) {
	assert(spdr);
	assert(handle);

	auto ch_ptr = new chan<bool>;
	std::unique_ptr<chan<bool>> ch_uptr(ch_ptr);
	async(
		handle, [=](bool r) mutable { *ch_ptr << r; }, timeout);
	return ch_ptr->pop(std::move(spdr));
}

inline bool await(HANDLE handle, duration timeout = duration_max()) {
	assert(handle);

	auto spdr = this_suspender();
	if (spdr.type_is<rua::thread_suspender>()) {
		return WaitForSingleObject(
				   handle, timeout.milliseconds<DWORD, INFINITE>()) !=
			   WAIT_TIMEOUT;
	}
	return await(std::move(spdr), handle, timeout);
}

} // namespace _wait

using namespace _wait;

}} // namespace rua::win32

#endif
