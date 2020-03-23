#ifndef _RUA_SYNC_MUTEX_HPP
#define _RUA_SYNC_MUTEX_HPP

#include "lockfree_list.hpp"

#include "../chrono/tick.hpp"
#include "../sched/scheduler.hpp"
#include "../types/util.hpp"

#include <atomic>
#include <cassert>
#include <queue>

namespace rua {

class mutex {
public:
	constexpr mutex() : _locked(false), _waiters(), _wkr_c(0) {}

	mutex(const mutex &) = delete;

	mutex &operator=(const mutex &) = delete;

	bool try_lock(ms timeout = 0) {
		if (!_locked.exchange(true)) {
			return true;
		}
		if (!timeout) {
			return false;
		}

		auto sch = this_scheduler();
		auto wkr = sch->get_waker();
		auto it = _waiters.emplace_front(wkr);

		if (it.is_back() && !_locked.exchange(true)) {
			_waiters.erase(it);
			return true;
		}

		if (timeout == duration_max()) {
			for (;;) {
				if (sch->sleep(timeout, true)) {
					if (!_locked.exchange(true)) {
						return true;
					}
					it = _waiters.emplace_front(wkr);
				}
			}
		}

		for (;;) {
			auto t = tick();
			auto r = sch->sleep(timeout, true);
			if (!_locked.exchange(true)) {
				return true;
			}
			if (!r) {
				return false;
			}
			timeout -= tick() - t;
			if (timeout <= 0) {
				return false;
			}
			it = _waiters.emplace_front(wkr);
		}
		return false;
	}

	void lock() {
		try_lock(duration_max());
	}

	void unlock() {
		auto waiter_opt = _waiters.pop_back();

#ifndef NDEBUG
		assert(_locked.exchange(false));
#else
		_locked.store(false);
#endif

		if (!waiter_opt) {
			return;
		}
		waiter_opt.value()->wake();
	}

private:
	std::atomic<bool> _locked;
	lockfree_list<waker_i> _waiters;
	std::atomic<size_t> _wkr_c;
};

} // namespace rua

#endif
