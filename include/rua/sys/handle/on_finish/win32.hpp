#ifndef _RUA_SYS_HANDLE_ON_FINISH_WIN32_HPP
#define _RUA_SYS_HANDLE_ON_FINISH_WIN32_HPP

#include "../../../chrono.hpp"
#include "../../../macros.hpp"
#include "../../../types/util.hpp"

#include <cassert>
#include <functional>

#include <winbase.h>

namespace rua { namespace win32 {

struct _async_wait_sys_h_ctx_t {
	HANDLE wait_h;
	std::function<void(bool)> cb;
};

inline VOID CALLBACK _async_wait_sys_h_cb(PVOID _ctx, BOOLEAN timeouted) {
	auto ctx = reinterpret_cast<_async_wait_sys_h_ctx_t *>(_ctx);
	ctx->cb(!timeouted);
	UnregisterWaitEx(ctx->wait_h, nullptr);
	delete ctx;
}

namespace _on_sys_handle_finish {

template <
	typename Callback,
	typename Ret = decltype(std::declval<Callback &&>()(true))>
inline enable_if_t<
	!std::is_function<remove_reference_t<Callback>>::value &&
		std::is_same<Ret, void>::value,
	Ret>
on_sys_handle_finish(
	HANDLE handle, Callback &&callback, duration timeout = duration_max()) {
	assert(handle);

	auto ctx = new _async_wait_sys_h_ctx_t{nullptr, std::move(callback)};

	RegisterWaitForSingleObject(
		&ctx->wait_h,
		handle,
		_async_wait_sys_h_cb,
		ctx,
		timeout.milliseconds<DWORD, INFINITE>(),
		WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE);
}

template <typename Ret, typename... Args>
inline Ret on_sys_handle_finish(
	HANDLE handle, Ret (&callee)(Args...), duration timeout = duration_max()) {
	return on_sys_handle_finish(handle, &callee, timeout);
}

} // namespace _on_sys_handle_finish

using namespace _on_sys_handle_finish;

}} // namespace rua::win32

#endif
