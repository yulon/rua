#ifndef _RUA_SYNC_MUTEX_HPP
#define _RUA_SYNC_MUTEX_HPP

#include "tsque.hpp"

#include "../sched.hpp"

#include <atomic>
#include <cstdint>
#include <queue>

namespace rua {

class mutex {
public:
	constexpr mutex() : _locked(false), _waiters() {}

	mutex(const mutex &) = delete;

	mutex &operator=(const mutex &) = delete;

	bool try_lock(ms timeout = 0) {
		if (!_locked.exchange(true)) {
			return true;
		}
		if (!timeout) {
			return false;
		}

		auto sch = get_scheduler();
		auto sig = sch->make_signaler();
		auto n = _waiters.emplace(sig);

		if (!_locked.exchange(true)) {
			if (!_waiters.erase(n)) {
				sig->reset();
			}
			return true;
		}

		return sch->wait(sig, timeout);
	}

	void lock() {
		while (!try_lock(duration_max()))
			;
	}

	void unlock() {
		auto waiter_opt = _waiters.pop();
		if (!waiter_opt.has_value()) {
			_locked.store(false);
			return;
		}
		waiter_opt.value()->signal();
	}

private:
	std::atomic<bool> _locked;
	tsque<scheduler::signaler_i> _waiters;
};

} // namespace rua

#endif
