#ifndef _RUA_SYS_LISTEN_WIN32_HPP
#define _RUA_SYS_LISTEN_WIN32_HPP

#include "../../macros.hpp"
#include "../../time.hpp"
#include "../../types/util.hpp"

#include <cassert>
#include <functional>

#include <winbase.h>

namespace rua { namespace win32 {

struct _sys_listen_ctx_t {
	HANDLE wait_h;
	std::function<void(bool)> cb;
};

inline VOID CALLBACK _sys_listen_cb(PVOID _ctx, BOOLEAN timeouted) {
	auto ctx = reinterpret_cast<_sys_listen_ctx_t *>(_ctx);
	ctx->cb(!timeouted);
	UnregisterWaitEx(ctx->wait_h, nullptr);
	delete ctx;
}

namespace _sys_listen {

inline void sys_listen(
	HANDLE handle,
	std::function<void(bool)> callback,
	duration timeout = duration_max()) {
	assert(handle);

	auto ctx = new _sys_listen_ctx_t{nullptr, std::move(callback)};

	RegisterWaitForSingleObject(
		&ctx->wait_h,
		handle,
		_sys_listen_cb,
		ctx,
		timeout.milliseconds<DWORD, INFINITE>(),
		WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE);
}

} // namespace _sys_listen

using namespace _sys_listen;

}} // namespace rua::win32

#endif
