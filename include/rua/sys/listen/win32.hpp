#ifndef _RUA_SYS_LISTEN_WIN32_HPP
#define _RUA_SYS_LISTEN_WIN32_HPP

#include "../../sync/promise.hpp"
#include "../../sync/wait.hpp"
#include "../../time.hpp"
#include "../../util.hpp"

#include <cassert>
#include <functional>

#include <winbase.h>

namespace rua { namespace win32 {

class sys_waiter : public waiter<sys_waiter> {
public:
	using native_handle_t = HANDLE;

	////////////////////////////////////////////////////////////////

	constexpr sys_waiter() : _h(nullptr), _wh(nullptr), _fut() {}

	explicit sys_waiter(HANDLE h) : _h(h), _wh(nullptr), _fut() {}

	~sys_waiter() {
		reset();
	}

	sys_waiter(sys_waiter &&src) :
		_h(exchange(src._h, nullptr)),
		_wh(exchange(src._wh, nullptr)),
		_fut(std::move(src._fut)) {}

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
		promise<> prm;
		_fut = prm.get_future();
		_fut.await_suspend(std::move(resume));
		auto prm_ctx = prm.release();
		RegisterWaitForSingleObject(
			&_wh,
			_h,
			_rw4so_cb,
			prm_ctx,
			INFINITE,
			WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE);
	}

	void await_resume() const {}

	void reset() {
		if (_h) {
			_h = nullptr;
		}
		if (!_fut) {
			return;
		}
		assert(_wh);
		_fut.checkout_or_lose_promise(!UnregisterWaitEx(_wh, nullptr));
		_wh = nullptr;
	}

private:
	HANDLE _h, _wh;
	future<> _fut;

	static VOID CALLBACK _rw4so_cb(PVOID prm_ctx, BOOLEAN /* timeouted */) {
		promise<>(*reinterpret_cast<promise_context<> *>(prm_ctx)).set_value();
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
