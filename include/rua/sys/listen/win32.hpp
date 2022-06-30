#ifndef _RUA_SYS_LISTEN_WIN32_HPP
#define _RUA_SYS_LISTEN_WIN32_HPP

#include "../../sync/await.hpp"
#include "../../sync/promise.hpp"
#include "../../sync/then.hpp"
#include "../../sync/wait.hpp"
#include "../../time.hpp"
#include "../../util.hpp"

#include <cassert>
#include <functional>

#include <winbase.h>

namespace rua { namespace win32 {

class sys_waiter : private enable_await_operators {
public:
	using native_handle_t = HANDLE;

	////////////////////////////////////////////////////////////////

	constexpr sys_waiter() : _h(nullptr), _wh(nullptr), _pms_fut() {}

	explicit sys_waiter(HANDLE h) : _h(h), _wh(nullptr), _pms_fut() {}

	~sys_waiter() {
		reset();
	}

	sys_waiter(sys_waiter &&src) :
		_h(exchange(src._h, nullptr)),
		_wh(exchange(src._wh, nullptr)),
		_pms_fut(std::move(src._pms_fut)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(sys_waiter);

	native_handle_t native_handle() const {
		return _wh;
	}

	native_handle_t target() const {
		return _h;
	}

	bool await_ready() const {
		return WaitForSingleObject(_h, 0) != WAIT_TIMEOUT;
	}

	template <typename Resume>
	void await_suspend(Resume resume) {
		promise<> pms;
		_pms_fut = pms;
		_pms_fut.await_suspend(std::move(resume));
		auto pms_ctx = pms.release();
		RegisterWaitForSingleObject(
			&_wh,
			_h,
			_rw4so_cb,
			pms_ctx,
			INFINITE,
			WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE);
	}

	void await_resume() const {}

	void reset() {
		if (_h) {
			_h = nullptr;
		}
		if (!_pms_fut) {
			return;
		}
		assert(_wh);
		_pms_fut.checkout_or_lose_promise(!UnregisterWaitEx(_wh, nullptr));
		_wh = nullptr;
	}

private:
	HANDLE _h, _wh;
	promising_future<> _pms_fut;

	static VOID CALLBACK _rw4so_cb(PVOID pms_ctx, BOOLEAN /* timeouted */) {
		promise<>(*reinterpret_cast<promise_context<> *>(pms_ctx)).set_value();
	}
};

namespace _sys_listen {

inline sys_waiter sys_listen(HANDLE handle, std::function<void()> callback) {
	assert(handle);

	sys_waiter wtr(handle);
	if (wtr.await_ready()) {
		callback();
		return wtr;
	}
	wtr.await_suspend(std::move(callback));
	return wtr;
}

} // namespace _sys_listen

using namespace _sys_listen;

}} // namespace rua::win32

#endif
