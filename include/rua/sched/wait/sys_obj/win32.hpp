#ifndef _RUA_SCHED_WAIT_SYS_OBJ_WIN32_HPP
#define _RUA_SCHED_WAIT_SYS_OBJ_WIN32_HPP

#include "win32_async.hpp"

#include "../../scheduler/this_decl.hpp"

#include "../../../sync/chan.hpp"
#include "../../../thread/scheduler.hpp"

#include <cassert>
#include <memory>

namespace rua { namespace win32 {

namespace _wait_sys_obj {

inline bool wait(HANDLE handle, ms timeout = duration_max()) {
	assert(handle);

	auto sch = this_scheduler();
	if (sch.type_is<rua::thread_scheduler>()) {
		return WaitForSingleObject(
				   handle,
				   static_cast<int64_t>(nmax<DWORD>()) < timeout.count()
					   ? nmax<DWORD>()
					   : static_cast<DWORD>(timeout.count())) != WAIT_TIMEOUT;
	}
	auto ch = std::make_shared<chan<bool>>();
	_wait_sys_obj_async::wait(
		handle, [=](bool r) mutable { *ch << r; }, timeout);
	return ch->pop(sch);
}

} // namespace _wait_sys_obj

using namespace _wait_sys_obj;

}} // namespace rua::win32

#endif
