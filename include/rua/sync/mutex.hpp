#ifndef _RUA_SYNC_MUTEX_HPP
#define _RUA_SYNC_MUTEX_HPP

#include "lf_forward_list.hpp"

#include "../chrono/clock.hpp"
#include "../sched.hpp"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <queue>

namespace rua {

class mutex {
public:
	constexpr mutex() : _locked(false), _waiters(), _sig_c(0) {}

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
		auto sig = sch->get_signaler();
		auto it = _waiters.emplace_front(sig);

		if (it.is_back() && !_locked.exchange(true)) {
			_waiters.erase(it);
			return true;
		}

		if (timeout == duration_max()) {
			for (;;) {
				if (sch->wait(timeout)) {
					if (!_locked.exchange(true)) {
						return true;
					}
					it = _waiters.emplace_front(sig);
				}
			}
		}

		for (;;) {
			auto t = tick();
			auto r = sch->wait(timeout);
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
			it = _waiters.emplace_front(sig);
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
		waiter_opt.value()->signal();
	}

private:
	std::atomic<bool> _locked;
	lf_forward_list<scheduler::signaler_i> _waiters;
	std::atomic<size_t> _sig_c;
};

} // namespace rua

#endif
