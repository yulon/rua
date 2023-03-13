#ifndef _rua_sys_listen_win32_hpp
#define _rua_sys_listen_win32_hpp

#include "../../time.hpp"
#include "../../util.hpp"

#include <cassert>
#include <functional>

#include <winbase.h>

namespace rua { namespace win32 {

struct _sys_listen_ctx_t {
	HANDLE h, wh;
	std::function<void()> cb;
};

inline VOID CALLBACK _sys_listen_cb(PVOID ctx_vptr, BOOLEAN /* timeouted */) {
	auto ctx_ptr = reinterpret_cast<_sys_listen_ctx_t *>(ctx_vptr);
	ctx_ptr->cb();
	UnregisterWaitEx(ctx_ptr->wh, nullptr);
	delete ctx_ptr;
}

inline void _sys_listen_force(HANDLE handle, std::function<void()> callback) {
	assert(handle);

	auto ctx_ptr = new _sys_listen_ctx_t{handle, nullptr, std::move(callback)};

	RegisterWaitForSingleObject(
		&ctx_ptr->wh,
		handle,
		_sys_listen_cb,
		ctx_ptr,
		INFINITE,
		WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE);
}

namespace _sys_listen {

inline void sys_listen(HANDLE handle, std::function<void()> callback) {
	assert(handle);

	if (WaitForSingleObject(handle, 0) != WAIT_TIMEOUT) {
		callback();
		return;
	}
	_sys_listen_force(handle, std::move(callback));
}

} // namespace _sys_listen

using namespace _sys_listen;

}} // namespace rua::win32

#endif
