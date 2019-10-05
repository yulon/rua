#ifndef _RUA_SCHED_SYS_WAIT_SYNC_WIN32_HPP
#define _RUA_SCHED_SYS_WAIT_SYNC_WIN32_HPP

#include "../../util.hpp"
#include "../async/win32.hpp"

#include <cassert>
#include <memory>

namespace rua { namespace win32 {

namespace _sys_wait_sync {

inline bool sys_wait(HANDLE handle, ms timeout = duration_max()) {
	assert(handle);

	auto sch = this_scheduler();
	auto sig = sch->get_signaler();
	auto r_ptr = new bool;
	_sys_wait_async::sys_wait(
		handle,
		[=](bool r) {
			*r_ptr = r;
			sig->signal();
		},
		timeout);
	sch->wait();
	auto r = *r_ptr;
	delete r_ptr;
	return r;
}

} // namespace _sys_wait_sync

using namespace _sys_wait_sync;

}} // namespace rua::win32

#endif
