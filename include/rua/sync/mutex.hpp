#ifndef _RUA_SYNC_MUTEX_HPP
#define _RUA_SYNC_MUTEX_HPP

#include "lockfree_list.hpp"

#include "../chrono/tick.hpp"
#include "../sched/scheduler.hpp"
#include "../types/util.hpp"

#include <atomic>
#include <cassert>

namespace rua {

class mutex {
public:
	constexpr mutex() : _locked(false), _waiters() {}

	mutex(const mutex &) = delete;

	mutex &operator=(const mutex &) = delete;

	bool try_pop() {
		return !_waiters && !_locked.exchange(true);
	}

	bool try_lock(duration timeout) {
		if (!_waiters && !_locked.exchange(true)) {
			return true;
		}
		if (!timeout) {
			return false;
		}
		return _wait_and_lock(this_scheduler(), timeout);
	}

	bool try_lock(scheduler_i sch, duration timeout) {
		if (!_waiters && !_locked.exchange(true)) {
			return true;
		}
		if (!timeout) {
			return false;
		}
		return _wait_and_lock(std::move(sch), timeout);
	}

	void lock() {
		try_lock(duration_max());
	}

	void lock(scheduler_i sch) {
		try_lock(std::move(sch), duration_max());
	}

	void unlock() {
#ifdef NDEBUG
		_locked.store(false);
#else
		assert(_locked.exchange(false));
#endif
		auto waiter_opt =
			_waiters.pop_back_if([this]() -> bool { return !_locked.load(); });
		if (!waiter_opt) {
			return;
		}
		waiter_opt.value()->wake();
	}

private:
	std::atomic<bool> _locked;
	lockfree_list<waker_i> _waiters;

	bool _wait_and_lock(scheduler_i sch, duration timeout) {
		assert(sch);
		assert(timeout);

		auto wkr = sch->get_waker();

		if (!_waiters.emplace_front_if_non_empty_or(
				[this]() -> bool { return _locked.exchange(true); }, wkr)) {
			return true;
		}

		if (timeout == duration_max()) {
			for (;;) {
				if (sch->sleep(timeout, true) &&
					!_waiters.emplace_front_if_non_empty_or(
						[this]() -> bool { return _locked.exchange(true); },
						wkr)) {
					return true;
				}
			}
		}

		for (;;) {
			auto t = tick();
			auto r = sch->sleep(timeout, true);
			timeout -= tick() - t;
			if (timeout <= 0) {
				return r && !_locked.exchange(true);
			}
			if (r &&
				!_waiters.emplace_front_if_non_empty_or(
					[this]() -> bool { return _locked.exchange(true); }, wkr)) {
				return true;
			}
		}
		return false;
	}
};

} // namespace rua

#endif
