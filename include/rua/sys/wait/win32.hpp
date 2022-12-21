#ifndef _RUA_SYS_WAIT_WIN32_HPP
#define _RUA_SYS_WAIT_WIN32_HPP

#include "../../skater.hpp"
#include "../../sync/promise.hpp"

#include "../listen/win32.hpp"

namespace rua { namespace win32 {

namespace _sys_wait {

inline future<> sys_wait(HANDLE handle) {
	assert(handle);

	future<> r;

	if (WaitForSingleObject(handle, 0) != WAIT_TIMEOUT) {
		return r;
	}

	skater<promise<>> prm;
	r = prm->get_future();

	_sys_listen_force(handle, [prm]() mutable { prm->deliver(); });

	return r;
}

} // namespace _sys_wait

using namespace _sys_wait;

}} // namespace rua::win32

#endif
