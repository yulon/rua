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

		if (_waiters.emplace(wkr) && !_locked.exchange(true)) {
			_erase_waiter(wkr);
			return true;
		}

		if (timeout == duration_max()) {
			for (;;) {
				if (sch->sleep(timeout, true)) {
					if (!_locked.exchange(true)) {
						return true;
					}
					if (_waiters.emplace(wkr) && !_locked.exchange(true)) {
						_erase_waiter(wkr);
						return true;
					}
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
			if (_waiters.emplace(wkr) && !_locked.exchange(true)) {
				_erase_waiter(wkr);
				return true;
			}
		}
		return false;
	}

	void lock() {
		try_lock(duration_max());
	}

	void unlock() {
		auto waiters = _waiters.pop();
#ifdef NDEBUG
		_locked.store(false);
#else
		assert(_locked.exchange(false));
#endif
		while (waiters) {
			waiters.pop_front()->wake();
		}
	}

private:
	std::atomic<bool> _locked;
	lockfree_list<waker_i> _waiters;
	std::atomic<size_t> _wkr_c;

	void _erase_waiter(waker_i waiter) {
		auto waiters = _waiters.pop();
		auto before = waiters.end();
		auto after = waiters.begin();
		while (after) {
			if (*after == waiter) {
				if (!before) {
					waiters.erase_front();
					break;
				}
				waiters.erase_after(before);
				break;
			}
			before = after++;
		}
		if (waiters) {
			_waiters.prepend(std::move(waiters));
		}
	}
};

} // namespace rua

#endif
