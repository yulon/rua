#ifndef _RUA_SYNC_WAIT_HPP
#define _RUA_SYNC_WAIT_HPP

#include "../async/await.hpp"
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

template <typename Awaitable, typename Result = void>
class waiter {
public:
	optional<Result> try_wait(duration timeout = 0) & {
		return rua::try_wait(*_this(), timeout);
	}

	optional<Result> try_wait(duration timeout = 0) && {
		return rua::try_wait(*_this(), timeout);
	}

	optional<Result> try_wait(duration timeout = 0) const & {
		return rua::try_wait(*_this(), timeout);
	}

	Result wait() & {
		return rua::wait(*_this());
	}

	Result wait() && {
		return rua::wait(*_this());
	}

	Result wait() const & {
		return rua::wait(*_this());
	}

	Result operator*() & {
		return wait();
	}

	Result operator*() && {
		return wait();
	}

	Result operator*() const & {
		return wait();
	}

private:
	const Awaitable *_this() const {
		return static_cast<const Awaitable *>(this);
	}

	Awaitable *_this() {
		return static_cast<Awaitable *>(this);
	}
};

template <typename Awaitable>
class waiter<Awaitable, void> {
public:
	bool try_wait(duration timeout = 0) {
		return rua::try_wait(*_this(), timeout);
	}

	bool try_wait(duration timeout = 0) const {
		return rua::try_wait(*_this(), timeout);
	}

	void wait() {
		rua::wait(*_this());
	}

	void wait() const {
		rua::wait(*_this());
	}

	void operator*() {
		wait();
	}

	void operator*() const {
		wait();
	}

private:
	const Awaitable *_this() const {
		return static_cast<const Awaitable *>(this);
	}

	Awaitable *_this() {
		return static_cast<Awaitable *>(this);
	}
};

} // namespace rua

#endif
