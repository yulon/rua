#ifndef _rua_sync_wait_hpp
#define _rua_sync_wait_hpp

#include "await.hpp"

#include "../optional.hpp"
#include "../thread/dozer.hpp"
#include "../time/tick.hpp"

#include <cassert>

namespace rua {

template <typename Awaitable, typename Result = await_result_t<Awaitable &&>>
inline Result wait(Awaitable &&awaitable) {
	auto &&awaiter = make_awaiter(std::forward<Awaitable>(awaitable));
	if (awaiter.await_ready()) {
		return awaiter.await_resume();
	}
	dozer dzr;
	auto wkr_wptr = dzr.get_waker();
	if (await_suspend(awaiter, [wkr_wptr]() {
			auto wkr_sp = wkr_wptr.lock();
			assert(wkr_sp);
			wkr_sp->wake();
		})) {
		while (!dzr.doze())
			;
	}
	return awaiter.await_resume();
}

namespace await_operators {

template <typename Awaitable, typename Result = await_result_t<Awaitable &&>>
inline Result operator*(Awaitable &&awaitable) {
	return wait<Awaitable, Result>(std::forward<Awaitable>(awaitable));
}

} // namespace await_operators

template <typename Awaitable, typename Result = await_result_t<Awaitable &&>>
inline enable_if_t<!std::is_void<Result>::value, optional<Result>>
try_wait(Awaitable &&awaitable, duration timeout = 0) {
	if (timeout == duration_max()) {
		return wait(std::forward<Awaitable>(awaitable));
	}

	auto &&awaiter = make_awaiter(std::forward<Awaitable>(awaitable));
	if (awaiter.await_ready()) {
		return awaiter.await_resume();
	}
	if (!timeout) {
		return nullopt;
	}

	dozer dzr;
	auto wkr_wptr = dzr.get_waker();
	if (!await_suspend(awaiter, [wkr_wptr]() {
			auto wkr_sp = wkr_wptr.lock();
			assert(wkr_sp);
			wkr_sp->wake();
		})) {
		return awaiter.await_resume();
	}

	auto end_ti = tick() + timeout;
	for (;;) {
		auto ok = dzr.doze(timeout);
		if (ok) {
			return awaiter.await_resume();
		}
		if (!ok || assign(timeout, end_ti - tick()) <= 0) {
			break;
		}
	}
	return nullopt;
}

template <typename Awaitable, typename Result = await_result_t<Awaitable &&>>
inline enable_if_t<std::is_void<Result>::value, bool>
try_wait(Awaitable &&awaitable, duration timeout = 0) {
	if (timeout == duration_max()) {
		wait(std::forward<Awaitable>(awaitable));
		return true;
	}

	auto &&awaiter = make_awaiter(std::forward<Awaitable>(awaitable));
	if (awaiter.await_ready()) {
		awaiter.await_resume();
		return true;
	}
	if (!timeout) {
		return false;
	}

	dozer dzr;
	auto wkr_wptr = dzr.get_waker();
	if (!await_suspend(awaiter, [wkr_wptr]() {
			auto wkr_sp = wkr_wptr.lock();
			assert(wkr_sp);
			wkr_sp->wake();
		})) {
		awaiter.await_resume();
		return true;
	}

	auto end_ti = tick() + timeout;
	for (;;) {
		auto ok = dzr.doze(timeout);
		if (ok) {
			awaiter.await_resume();
			return true;
		}
		if (!ok || assign(timeout, end_ti - tick()) <= 0) {
			break;
		}
	}
	return false;
}

} // namespace rua

#endif
