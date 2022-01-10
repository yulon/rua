#ifndef _RUA_SYNC_MUTEX_HPP
#define _RUA_SYNC_MUTEX_HPP

#include "lockfree_list.hpp"

#include "../sched/dozer.hpp"
#include "../time/tick.hpp"
#include "../types/util.hpp"

#include <atomic>
#include <cassert>

namespace rua {

class mutex {
public:
	constexpr mutex() : _locked(0), _waiters() {}

	mutex(const mutex &) = delete;

	mutex &operator=(const mutex &) = delete;

	bool try_lock() {
		auto old_locked = _locked.load();
		while (!old_locked) {
			if (_locked.compare_exchange_weak(old_locked, nmax<uintptr_t>())) {
				return true;
			}
		}
		return false;
	}

	bool try_lock(duration timeout) {
		if (try_lock()) {
			return true;
		}
		if (!timeout) {
			return false;
		}
		return _wait_and_lock(this_dozer(), timeout);
	}

	bool try_lock(dozer_i dzr, duration timeout) {
		if (try_lock()) {
			return true;
		}
		if (!timeout) {
			return false;
		}
		return _wait_and_lock(std::move(dzr), timeout);
	}

	void lock() {
		try_lock(duration_max());
	}

	void lock(dozer_i dzr) {
		try_lock(std::move(dzr), duration_max());
	}

	void unlock() {
		auto waiters = _waiters.lock();
		if (waiters.empty()) {
#ifdef NDEBUG
			_locked.store(0);
#else
			assert(_locked.exchange(0));
#endif
			_waiters.unlock();
			return;
		}
		auto waiter = waiters.pop_back();
		_waiters.unlock_and_prepend(std::move(waiters));
#ifdef NDEBUG
		_locked.store(reinterpret_cast<uintptr_t>(waiter.get()));
#else
		assert(_locked.exchange(reinterpret_cast<uintptr_t>(waiter.get())));
#endif
		waiter->wake();
	}

private:
	std::atomic<uintptr_t> _locked;
	lockfree_list<waker_i> _waiters;

	bool _wait_and_lock(dozer_i dzr, duration timeout) {
		assert(dzr);
		assert(timeout);

		auto wkr = dzr->get_waker();
		auto wkr_id = reinterpret_cast<uintptr_t>(wkr.get());

		if (!_waiters.emplace_front_if(
				[this, wkr_id]() -> bool { return _locked.load() != wkr_id; },
				wkr)) {
			return true;
		}

		if (timeout == duration_max()) {
			for (;;) {
				if (dzr->doze(timeout) && (_locked.load() == wkr_id ||
										   !_waiters.emplace_front_if(
											   [this, wkr_id]() -> bool {
												   return _locked.load() !=
														  wkr_id;
											   },
											   wkr))) {
					return true;
				}
			}
		}

		for (;;) {
			auto t = tick();
			auto r = dzr->doze(timeout);
			timeout -= tick() - t;
			if (timeout <= 0) {
				return _locked.load() == wkr_id;
			}
			if (r && (_locked.load() == wkr_id ||
					  !_waiters.emplace_front_if(
						  [this, wkr_id]() -> bool {
							  return _locked.load() != wkr_id;
						  },
						  wkr))) {
				return true;
			}
		}
		return false;
	}
};

} // namespace rua

#endif
