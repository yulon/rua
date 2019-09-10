#ifndef _RUA_SCHED_SYSWAIT_SYNC_WIN32_HPP
#define _RUA_SCHED_SYSWAIT_SYNC_WIN32_HPP

#include "../../util.hpp"
#include "../async/win32.hpp"

#include <cassert>
#include <memory>

namespace rua { namespace win32 {

namespace _syswait_sync {

inline bool syswait(HANDLE handle, ms timeout = duration_max()) {
	assert(handle);

	auto sch = get_scheduler();
	auto sig = sch->make_signaler();
	auto r_ptr = new bool;
	_syswait_async::syswait(handle, timeout, [=](bool r) {
		*r_ptr = r;
		sig->signal();
	});
	sch->wait(sig);
	auto r = *r_ptr;
	delete r_ptr;
	return r;
}

} // namespace _syswait_sync

using namespace _syswait_sync;

}} // namespace rua::win32

#endif
